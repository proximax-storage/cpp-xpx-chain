/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#include "AsyncTcpServer.h"
#include "ConnectionSettings.h"
#include "catapult/thread/IoThreadPool.h"

namespace catapult { namespace net {

	namespace {
		/// Allow at most one pending accept at a time.
		const uint32_t Max_Pending_Accepts = 1;

		void EnableAddressReuse(boost::asio::ip::tcp::acceptor& acceptor) {
			acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));

#ifndef _WIN32
			using reuse_port = boost::asio::detail::socket_option::boolean<BOOST_ASIO_OS_DEF(SOL_SOCKET), SO_REUSEPORT>;
			acceptor.set_option(reuse_port(true));
#endif
		}

		void BindAcceptor(
				boost::asio::ip::tcp::acceptor& acceptor,
				const boost::asio::ip::tcp::endpoint& endpoint,
				bool allowAddressReuse) {
			acceptor.open(endpoint.protocol());

			if (allowAddressReuse) {
				CATAPULT_LOG(info) << "configuring AsyncTcpServer to reuse addresses";
				EnableAddressReuse(acceptor);
			}

			acceptor.bind(endpoint);
		}

		class DefaultAsyncTcpServer
				: public AsyncTcpServer
				, public std::enable_shared_from_this<DefaultAsyncTcpServer> {
		public:
			DefaultAsyncTcpServer(
					const std::shared_ptr<thread::IoThreadPool>& pPool,
					const boost::asio::ip::tcp::endpoint& endpoint,
					const AsyncTcpServerSettings& settings)
					: m_pPool(pPool)
					, m_acceptorStrand(pPool->ioContext())
					, m_acceptor(pPool->ioContext())
					, m_settings(settings)
					, m_isStopped(false)
					, m_numPendingAccepts(0)
					, m_numCurrentConnections(0)
					, m_numLifetimeConnections(0) {
				BindAcceptor(m_acceptor, endpoint, m_settings.AllowAddressReuse);
				CATAPULT_LOG(info) << "AsyncTcpServer created around " << endpoint;
			}

			~DefaultAsyncTcpServer() override {
				shutdown();
			}

		public:
			uint32_t numPendingAccepts() const override {
				return m_numPendingAccepts;
			}

			uint32_t numCurrentConnections() const override {
				return m_numCurrentConnections;
			}

			uint32_t numLifetimeConnections() const override {
				return m_numLifetimeConnections;
			}

		public:
			void start() {
				m_acceptor.listen(m_settings.MaxPendingConnections);
				tryStartAccept();

				CATAPULT_LOG(trace) << "AsyncTcpServer waiting for threads to enter pending accept state";
				while (m_numPendingAccepts < Max_Pending_Accepts) {}
				CATAPULT_LOG(info) << "AsyncTcpServer spawned " << m_numPendingAccepts << " pending accepts";
			}

			void shutdown() override {
				bool expectedIsStopped = false;
				if (!m_isStopped.compare_exchange_strong(expectedIsStopped, true))
					return;

				// close the acceptor to prevent new connections and block until the close actually happens
				CATAPULT_LOG(info) << "AsyncTcpServer stopping";
				boost::asio::dispatch(m_acceptorStrand, [pThis = shared_from_this()]() {
					pThis->closeAcceptor();
				});

				while (0 != m_numPendingAccepts) {}
				CATAPULT_LOG(info) << "AsyncTcpServer stopped";
			}

		private:
			void closeAcceptor() {
				boost::system::error_code ignored_ec;
				m_acceptor.close(ignored_ec);
			}

			void handleAccept(const ionet::AcceptedPacketSocketInfo& socketInfo) {
				// add a destruction hook to the socket and post additional handling to the strand
				ionet::AcceptedPacketSocketInfo decoratedSocketInfo(socketInfo.host(), addDestructionHook(socketInfo.socket()));
				boost::asio::post(m_acceptorStrand, [pThis = shared_from_this(), decoratedSocketInfo]() {
					pThis->handleAcceptOnStrand(decoratedSocketInfo);
				});
			}

			void handleAcceptOnStrand(const ionet::AcceptedPacketSocketInfo& socketInfo) {
				// decrement the number of pending accepts
				--m_numPendingAccepts;

				// if accept had an error, try to start an accept and exit
				if (!socketInfo) {
					tryStartAccept();
					return;
				}

				// if accept succeeded, increment connection counters and try to start an accept
				++m_numLifetimeConnections;
				++m_numCurrentConnections;
				tryStartAccept();

				// post the user callback on the thread pool (outside of the strand)
				boost::asio::post(m_pPool->ioContext(), [userCallback = m_settings.Accept, socketInfo] {
					userCallback(socketInfo);
				});
			}

			std::shared_ptr<ionet::PacketSocket> addDestructionHook(const std::shared_ptr<ionet::PacketSocket>& pSocket) {
				return {pSocket.get(), [pSocket, pThis = shared_from_this()](auto* pRawSocket) {
					if (!pRawSocket)
						return;

					pRawSocket->close();

					// if a valid connection was wrapped, decrement the number of current connections and attempt to start a new accept
					boost::asio::post(pThis->m_acceptorStrand, [pThis] {
						pThis->handleContextDestructionOnStrand();
					});
				}};
			}

			void handleContextDestructionOnStrand() {
				--m_numCurrentConnections;
				tryStartAccept();
			}

			// note that aside from start, which blocks, this function is always called from within a strand,
			// so no additional synchronization is necessary inside
			void tryStartAccept() {
				if (m_isStopped) {
					CATAPULT_LOG(trace) << "bypassing Accept because server is stopping";
					return;
				}

				uint32_t numActiveConnections = m_numPendingAccepts + m_numCurrentConnections;
				uint32_t numOpenConnectionSlots = m_settings.MaxActiveConnections - numActiveConnections;
				if (m_numPendingAccepts >= Max_Pending_Accepts || numOpenConnectionSlots <= 0) {
					CATAPULT_LOG(debug)
							<< "bypassing Accept due to limit (numPendingAccepts="
							<< m_numPendingAccepts << ", numOpenConnectionSlots=" << numOpenConnectionSlots << ")";
					return;
				}

				// start a new accept
				++m_numPendingAccepts;
				ionet::Accept(m_acceptor, m_settings.PacketSocketOptions, m_settings.ConfigureSocket, [pThis = shared_from_this()](
						const auto& socketInfo) {
					pThis->handleAccept(socketInfo);
				});
			}

		private:
			std::shared_ptr<thread::IoThreadPool> m_pPool;
			boost::asio::io_context::strand m_acceptorStrand;
			boost::asio::ip::tcp::acceptor m_acceptor;

			const AsyncTcpServerSettings m_settings;
			std::atomic_bool m_isStopped;
			std::atomic<uint32_t> m_numPendingAccepts;
			std::atomic<uint32_t> m_numCurrentConnections;
			std::atomic<uint32_t> m_numLifetimeConnections;
		};

		class SslAsyncTcpServer
				: public AsyncTcpServer
				, public std::enable_shared_from_this<SslAsyncTcpServer> {
		public:
			SslAsyncTcpServer(
					const std::shared_ptr<thread::IoThreadPool>& pPool,
					const boost::asio::ip::tcp::endpoint& endpoint,
					const AsyncSslTcpServerSettings& settings)
					: m_ioContext(pPool->ioContext())
					, m_acceptorStrand(m_ioContext)
					, m_acceptor(m_ioContext)
					, m_settings(settings)
					, m_isStopped(false)
					, m_hasPendingAccept(false)
					, m_numCurrentConnections(0)
					, m_numLifetimeConnections(0) {
				BindAcceptor(m_acceptor, endpoint, m_settings.AllowAddressReuse);
				CATAPULT_LOG(info) << "AsyncTcpServer created around " << endpoint;
			}

			~SslAsyncTcpServer() override {
				shutdown();
			}

		public:
			uint32_t numPendingAccepts() const override {
				return m_hasPendingAccept ? 1 : 0;
			}

			uint32_t numCurrentConnections() const override {
				return m_numCurrentConnections;
			}

			uint32_t numLifetimeConnections() const override {
				return m_numLifetimeConnections;
			}

		public:
			void start() {
				m_acceptor.listen(m_settings.MaxPendingConnections);
				tryStartAccept();

				CATAPULT_LOG(trace) << "AsyncTcpServer waiting for threads to enter pending accept state";
				while (!m_hasPendingAccept) {}
				CATAPULT_LOG(info) << "AsyncTcpServer spawned pending accept";
			}

			void shutdown() override {
				bool expectedIsStopped = false;
				if (!m_isStopped.compare_exchange_strong(expectedIsStopped, true))
					return;

				// close the acceptor to prevent new connections and block until the close actually happens
				CATAPULT_LOG(info) << "AsyncTcpServer stopping";
				boost::asio::dispatch(m_acceptorStrand, [pThis = shared_from_this()]() {
					pThis->closeAcceptor();
				});

				while (m_hasPendingAccept) {}
				CATAPULT_LOG(info) << "AsyncTcpServer stopped";
			}

		private:
			void closeAcceptor() {
				boost::system::error_code ignoredEc;
				m_acceptor.close(ignoredEc);
			}

			void handleAccept(const ionet::SslPacketSocketInfo& socketInfo) {
				// add a destruction hook to the socket and post additional handling to the strand
				ionet::SslPacketSocketInfo decoratedSocketInfo(
						socketInfo.host(),
						socketInfo.publicKey(),
						addDestructionHook(socketInfo.socket()));
				boost::asio::post(m_acceptorStrand, [pThis = shared_from_this(), decoratedSocketInfo]() {
					pThis->handleAcceptOnStrand(decoratedSocketInfo);
				});
			}

			void handleAcceptOnStrand(const ionet::SslPacketSocketInfo& socketInfo) {
				m_hasPendingAccept = false;

				// if accept had an error, try to start an accept and exit
				if (!socketInfo) {
					tryStartAccept();
					return;
				}

				// if accept succeeded, increment connection counters and try to start an accept
				++m_numLifetimeConnections;
				++m_numCurrentConnections;
				tryStartAccept();

				// post the user callback on the thread pool (outside of the strand)
				boost::asio::post(m_ioContext, [userCallback = m_settings.Accept, socketInfo] {
					userCallback(socketInfo);
				});
			}

			std::shared_ptr<ionet::SslPacketSocket> addDestructionHook(const std::shared_ptr<ionet::SslPacketSocket>& pSocket) {
				return {pSocket.get(), [pSocket, pThis = shared_from_this()](auto* pRawSocket) {
					if (!pRawSocket)
						return;

					pRawSocket->close();

					// if a valid connection was wrapped, decrement the number of current connections and attempt to start a new accept
					boost::asio::post(pThis->m_acceptorStrand, [pThis] {
						pThis->handleContextDestructionOnStrand();
					});
				}};
			}

			void handleContextDestructionOnStrand() {
				--m_numCurrentConnections;
				tryStartAccept();
			}

			// note that aside from start, which blocks, this function is always called from within a strand,
			// so no additional synchronization is necessary inside
			void tryStartAccept() {
				if (m_isStopped) {
					CATAPULT_LOG(trace) << "bypassing Accept because server is stopping";
					return;
				}

				uint32_t numActiveConnections = numPendingAccepts() + m_numCurrentConnections;
				uint32_t numOpenConnectionSlots = m_settings.MaxActiveConnections - numActiveConnections;
				if (numOpenConnectionSlots <= 0) {
					CATAPULT_LOG(debug) << "bypassing Accept due to limit (numOpenConnectionSlots=" << numOpenConnectionSlots << ")";
					return;
				}

				if (m_hasPendingAccept) {
					CATAPULT_LOG(trace) << "bypassing Accept due to current outstanding accept";
					return;
				}

				// start a new accept
				m_hasPendingAccept = true;
				ionet::Accept(m_ioContext, m_acceptor, m_settings.PacketSocketOptions, [pThis = shared_from_this()](const auto& socketInfo) {
					pThis->handleAccept(socketInfo);
				});
			}

		private:
			boost::asio::io_context& m_ioContext;
			boost::asio::io_context::strand m_acceptorStrand;
			boost::asio::ip::tcp::acceptor m_acceptor;

			const AsyncSslTcpServerSettings m_settings;
			std::atomic_bool m_isStopped;
			std::atomic_bool m_hasPendingAccept;
			std::atomic<uint32_t> m_numCurrentConnections;
			std::atomic<uint32_t> m_numLifetimeConnections;
		};
	}

	std::shared_ptr<AsyncTcpServer> CreateAsyncTcpServer(
			const std::shared_ptr<thread::IoThreadPool>& pPool,
			const boost::asio::ip::tcp::endpoint& endpoint,
			const AsyncTcpServerSettings& settings) {
		auto pServer = std::make_shared<DefaultAsyncTcpServer>(pPool, endpoint, settings);
		pServer->start();
		return std::move(pServer);
	}

	std::shared_ptr<AsyncTcpServer> CreateAsyncSslTcpServer(
			const std::shared_ptr<thread::IoThreadPool>& pPool,
			const boost::asio::ip::tcp::endpoint& endpoint,
			const AsyncSslTcpServerSettings& settings) {
		auto pServer = std::make_shared<SslAsyncTcpServer>(pPool, endpoint, settings);
		pServer->start();
		return std::move(pServer);
	}
}}
