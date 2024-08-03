/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "PacketReadersWriters.h"
#include "SslClientConnector.h"
#include "SslServerConnector.h"
#include "ChainedSocketReader.h"
#include "catapult/crypto/KeyPair.h"
#include "catapult/extensions/ServiceState.h"
#include "catapult/ionet/SslPacketSocket.h"
#include "catapult/ionet/SocketReader.h"
#include "catapult/thread/IoThreadPool.h"
#include "catapult/utils/ModificationSafeIterableContainer.h"

namespace catapult { namespace net {

	namespace {
		using SocketPointer = std::shared_ptr<ionet::SslPacketSocket>;

		struct ReaderWriterState {
			ionet::Node Node;
			SocketPointer pSocket;
			std::shared_ptr<ionet::PacketIo> pBufferedIo;
			std::shared_ptr<ChainedSocketReader> pReader;
		};

		// expected sequences
		// accept : insert -> remove
		// connect: prepareConnect -> abortConnect
		// connect: prepareConnect -> insert -> remove
		class WriterContainer {
		private:
			using Writers = utils::ModificationSafeIterableContainer<std::list<ReaderWriterState>>;

		public:
			size_t size() const {
				utils::SpinLockGuard guard(m_lock);
				return m_writers.size();
			}

			size_t numOutgoingConnections() const {
				utils::SpinLockGuard guard(m_lock);
				return m_outgoingNodeIdentityKeys.size();
			}

			utils::KeySet identities() const {
				utils::SpinLockGuard guard(m_lock);
				return m_nodeIdentityKeys;
			}

			bool pickOne(ReaderWriterState& state, const Key& identityKey) {
				utils::SpinLockGuard guard(m_lock);
				auto* pState = m_writers.nextIf([&identityKey](const ReaderWriterState& state) {
					return (state.Node.identityKey() == identityKey);
				});

				if (!pState)
					return false;

				state = *pState;
				return true;
			}

			bool prepareConnect(const ionet::Node& node) {
				utils::SpinLockGuard guard(m_lock);
				if (!m_outgoingNodeIdentityKeys.insert(node.identityKey()).second) {
					CATAPULT_LOG(debug) << "bypassing connection to already connected peer " << node;
					return false;
				}

				return true;
			}

			bool insert(const ReaderWriterState& state) {
				utils::SpinLockGuard guard(m_lock);

				// if the state is for an already connected node, ignore it
				// 1. required for filtering accepted connections
				// 2. prepareConnect proactively filters connections
				// 3. failsafe for mixed connect + accept use cases (not optimized)
				if (!m_nodeIdentityKeys.emplace(state.Node.identityKey()).second) {
					CATAPULT_LOG(debug) << "ignoring connection to already connected peer " << state.Node;
					return false;
				}

				m_writers.push_back(state);
				state.pReader->start();
				return true;
			}

			void abortConnect(const ionet::Node& node) {
				utils::SpinLockGuard guard(m_lock);
				CATAPULT_LOG(debug) << "aborting connection to: " << node;
				m_outgoingNodeIdentityKeys.erase(node.identityKey());
			}

			void remove(const SocketPointer& pSocket) {
				utils::SpinLockGuard guard(m_lock);
				auto iter = findStateBySocket(pSocket);
				if (m_writers.end() == iter) {
					CATAPULT_LOG(warning) << "ignoring request to remove unknown socket";
					return;
				}

				remove(iter);
			}

			bool close(const Key& identityKey) {
				utils::SpinLockGuard guard(m_lock);
				auto iter = findStateByKey(identityKey);
				if (m_writers.end() == iter)
					return false;

				CATAPULT_LOG(debug) << "closing connection to " << identityKey;
				iter->pSocket->close();
				remove(iter);
				return true;
			}

			void clear() {
				utils::SpinLockGuard guard(m_lock);
				m_nodeIdentityKeys.clear();
				m_outgoingNodeIdentityKeys.clear();
				m_writers.clear();
			}

		private:
			Writers::iterator findStateBySocket(const SocketPointer& pSocket) {
				return std::find_if(m_writers.begin(), m_writers.end(), [&pSocket](const auto& state) {
					return state.pSocket == pSocket;
				});
			}

			Writers::iterator findStateByKey(const Key& identityKey) {
				return std::find_if(m_writers.begin(), m_writers.end(), [&identityKey](const auto& state) {
					return state.Node.identityKey() == identityKey;
				});
			}

			void remove(Writers::iterator iter) {
				const auto& identityKey = iter->Node.identityKey();
				m_nodeIdentityKeys.erase(identityKey);
				m_outgoingNodeIdentityKeys.erase(identityKey);
				m_writers.erase(iter);
			}

		private:
			utils::KeySet m_nodeIdentityKeys; // keys of active writers (both connected AND accepted)
			utils::KeySet m_outgoingNodeIdentityKeys; // keys of connecting or connected writers
			Writers m_writers;
			mutable utils::SpinLock m_lock;
		};

		class ErrorHandlingPacketIo : public ionet::PacketIo {
		public:
			using ErrorCallback = action;

		public:
			ErrorHandlingPacketIo(
					const std::shared_ptr<ionet::PacketIo>& pPacketIo,
					const ErrorCallback& errorCallback)
					: m_pPacketIo(pPacketIo)
					, m_errorCallback(errorCallback)
			{}

		public:
			void read(const ReadCallback& callback) override {
				m_pPacketIo->read([callback, errorCallback = m_errorCallback](auto code, const auto* pPacket) {
					CheckError(code, errorCallback, "read");
					callback(code, pPacket);
				});
			}

			void write(const ionet::PacketPayload& payload, const WriteCallback& callback) override {
				m_pPacketIo->write(payload, [callback, errorCallback = m_errorCallback](auto code) {
					CheckError(code, errorCallback, "write");
					callback(code);
				});
			}

		private:
			static void CheckError(ionet::SocketOperationCode code, const ErrorCallback& handler, const char* operation) {
				if (ionet::SocketOperationCode::Success == code)
					return;

				CATAPULT_LOG(warning) << "calling error handler due to " << operation << " error " << code;
				handler();
			}

		private:
			std::shared_ptr<ionet::PacketIo> m_pPacketIo;
			// note that m_errorCallback is captured by value in the read / write callbacks to ensure that it
			// is always available even if the containing ErrorHandlingPacketIo is destroyed
			ErrorCallback m_errorCallback;
		};

		class DefaultPacketReadersWriters
				: public PacketReadersWriters
				, public std::enable_shared_from_this<DefaultPacketReadersWriters> {
		public:
			DefaultPacketReadersWriters(
					const std::shared_ptr<thread::IoThreadPool>& pPool,
					const ionet::ServerPacketHandlers& handlers,
					const crypto::KeyPair& keyPair,
					const ConnectionSettings& settings,
					extensions::ServiceState& state)
					: m_pPool(pPool)
					, m_handlers(handlers)
					, m_pClientConnector(CreateSslClientConnector(*m_pPool, keyPair, settings))
					, m_pServerConnector(CreateSslServerConnector(*m_pPool, keyPair, settings))
					, m_state(state)
			{}

		public:
			size_t numActiveConnections() const override {
				// use m_writers.numOutgoingConnections() instead of m_pServerConnector->numActiveConnections() because the latter
				// does not count pending connections (before socket CONNECT succeeds)
				return m_writers.numOutgoingConnections() + m_pClientConnector->numActiveConnections();
			}

			utils::KeySet identities() const override {
				return m_writers.identities();
			}

		public:
			void write(const Key& identityKey, const ionet::PacketPayload& payload, const WriteCallback& callback) override {
				ReaderWriterState state;
				if (!m_writers.pickOne(state, identityKey)) {
					CATAPULT_LOG(debug) << "no packet io available for sending " << payload.header() << " to " << identityKey;
					callback(ionet::SocketOperationCode::Closed);
					return;
				}

				// important - capture pSocket by value in order to prevent it from being removed out from under the
				// error handling packet io, also capture this for the same reason
				auto errorHandler = [pThis = shared_from_this(), pSocket = state.pSocket, node = state.Node]() {
					CATAPULT_LOG(warning) << "error handler triggered for " << node;
					pThis->removeWriter(pSocket);
				};

				auto pPacketIo = std::make_shared<ErrorHandlingPacketIo>(state.pBufferedIo, errorHandler);
				CATAPULT_LOG(trace) << "checked out an io for sending " << payload.header() << " to " << identityKey;

				pPacketIo->write(payload, callback);
			}

		public:
			void connect(const ionet::Node& node, const ConnectCallback& callback) override {
				if (!m_writers.prepareConnect(node))
					return callback(PeerConnectCode::Already_Connected);

				m_pServerConnector->connect(node, [pThis = shared_from_this(), node, callback](
						auto connectCode,
						const auto& verifiedSocketInfo) {
					// abort the connection if it failed or is redundant
					if (PeerConnectCode::Accepted != connectCode || !pThis->addWriter(node, verifiedSocketInfo)) {
						pThis->m_writers.abortConnect(node);

						if (PeerConnectCode::Accepted == connectCode)
							connectCode = PeerConnectCode::Already_Connected;
					}

					callback({ connectCode, node.identityKey() });
				});
			}

			void accept(const ionet::SslPacketSocketInfo& socketInfo, const ConnectCallback& callback) override {
				m_pClientConnector->accept(socketInfo, [pThis = shared_from_this(), host = socketInfo.host(), callback](
						auto connectCode,
						const auto& pVerifiedSocket,
						const auto& remoteKey) {
					ionet::SslPacketSocketInfo verifiedSocketInfo(host, remoteKey, pVerifiedSocket);
					if (PeerConnectCode::Accepted == connectCode) {
						if (!pThis->addWriter(remoteKey, verifiedSocketInfo)) {
							connectCode = PeerConnectCode::Already_Connected;
						} else {
							CATAPULT_LOG(debug) << "accepted connection from '" << verifiedSocketInfo.host() << "' as " << remoteKey;
						}
					}

					callback({ connectCode, remoteKey });
				});
			}

		private:
			bool addWriter(const Key& key, const ionet::SslPacketSocketInfo& socketInfo) {
				auto node = ionet::Node(key, ionet::NodeEndpoint(), ionet::NodeMetadata(m_state.networkIdentifier()));
				return addWriter(node, socketInfo);
			}

			bool addWriter(const ionet::Node& node, const ionet::SslPacketSocketInfo& socketInfo) {
				ReaderWriterState state;
				state.Node = node;
				state.pSocket = socketInfo.socket();
				state.pBufferedIo = state.pSocket->buffered();
				auto identity = ionet::ReaderIdentity{ node.identityKey(), socketInfo.host() };
				state.pReader = createReader(state.pSocket, state.pBufferedIo, identity);
				return m_writers.insert(state);
			}

			void removeWriter(const SocketPointer& pSocket) {
				pSocket->close();
				m_writers.remove(pSocket);
			}

			std::shared_ptr<ChainedSocketReader> createReader(
					const SocketPointer& pSocket,
					const std::shared_ptr<ionet::PacketIo>& pBufferedIo,
					const ionet::ReaderIdentity& identity) {
				const auto& identityKey = identity.PublicKey;
				return CreateChainedSocketReader(pSocket, pBufferedIo, m_handlers, identity, [pThis = shared_from_this(), identityKey](auto code) {
					pThis->closeOne(identityKey);
				});
			}

		public:
			bool closeOne(const Key& identityKey) override {
				return m_writers.close(identityKey);
			}

			void shutdown() override {
				CATAPULT_LOG(info) << "closing all connections in PacketReadersWriters";
				m_pClientConnector->shutdown();
				m_pServerConnector->shutdown();
				m_writers.clear();
			}

		private:
			std::shared_ptr<thread::IoThreadPool> m_pPool;
			const ionet::ServerPacketHandlers& m_handlers;
			std::shared_ptr<SslClientConnector> m_pClientConnector;
			std::shared_ptr<SslServerConnector> m_pServerConnector;
			extensions::ServiceState& m_state;
			WriterContainer m_writers;
		};
	}

	std::shared_ptr<PacketReadersWriters> CreatePacketReadersWriters(
			const std::shared_ptr<thread::IoThreadPool>& pPool,
			const ionet::ServerPacketHandlers& handlers,
			const crypto::KeyPair& keyPair,
			const ConnectionSettings& settings,
			extensions::ServiceState& state) {
		return std::make_shared<DefaultPacketReadersWriters>(pPool, handlers, keyPair, settings, state);
	}
}}
