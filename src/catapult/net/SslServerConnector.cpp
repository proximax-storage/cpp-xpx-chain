/**
*** Copyright (c) 2016-2019, Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp.
*** Copyright (c) 2020-present, Jaguar0625, gimre, BloodyRookie.
*** All rights reserved.
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

#include "SslServerConnector.h"
#include "VerifyPeer.h"
#include "catapult/crypto/KeyPair.h"
#include "catapult/ionet/Node.h"
#include "catapult/ionet/SslPacketSocket.h"
#include "catapult/thread/IoThreadPool.h"
#include "catapult/thread/TimedCallback.h"
#include "catapult/utils/Logging.h"
#include "catapult/utils/WeakContainer.h"

namespace catapult { namespace net {

	namespace {
		using PacketSocketPointer = std::shared_ptr<ionet::PacketSocket>;

		class DefaultSslServerConnector
				: public SslServerConnector
				, public std::enable_shared_from_this<DefaultSslServerConnector> {
		public:
			DefaultSslServerConnector(
					thread::IoThreadPool& pool,
					const crypto::KeyPair& keyPair,
					const ConnectionSettings& settings,
					const std::string& name)
					: m_ioContext(pool.ioContext())
					, m_keyPair(keyPair)
					, m_settings(settings)
					, m_name(name)
					, m_tag(m_name.empty() ? std::string() : " (" + m_name + ")")
					, m_sockets([](auto& socket) { socket.close(); })
			{}

		public:
			size_t numActiveConnections() const override {
				return m_sockets.size();
			}

			const std::string& name() const override {
				return m_name;
			}

		public:
			void connect(const ionet::Node& node, const SslConnectCallback& callback) override {
				if (!m_settings.AllowOutgoingSelfConnections && m_keyPair.publicKey() == node.identityKey()) {
					CATAPULT_LOG(warning) << "self connection detected and aborted" << m_tag;
					return callback(PeerConnectCode::Self_Connection_Error, ionet::SslPacketSocketInfo());
				}

				auto pRequest = thread::MakeTimedCallback(m_ioContext, callback, PeerConnectCode::Timed_Out, ionet::SslPacketSocketInfo());
				pRequest->setTimeout(m_settings.Timeout);

				auto socketOptions = m_settings.toSocketOptions();
				const auto& endpoint = node.endpoint();
				auto cancel = ionet::Connect(m_ioContext, socketOptions, endpoint, [pThis = shared_from_this(), node, pRequest](
						auto result,
						const auto& connectedSocketInfo) {
					if (ionet::ConnectResult::Connected != result)
						return pRequest->callback(PeerConnectCode::Socket_Error, ionet::SslPacketSocketInfo());

					pThis->verify(node, connectedSocketInfo, pRequest);
				});

				pRequest->setTimeoutHandler([pThis = shared_from_this(), cancel]() {
					cancel();
					CATAPULT_LOG(debug) << "connect failed due to timeout" << pThis->m_tag;
				});
			}

		private:
			template<typename TRequest>
			void verify(
					const ionet::Node& node,
					const ionet::SslPacketSocketInfo& connectedSocketInfo,
					const std::shared_ptr<TRequest>& pRequest) {
				ionet::SslPacketSocketInfo socketInfo(connectedSocketInfo.host(), node.identityKey(), connectedSocketInfo.socket());
				m_sockets.insert(connectedSocketInfo.socket());
				pRequest->setTimeoutHandler([pConnectedSocket = connectedSocketInfo.socket()]() {
					pConnectedSocket->close();
					CATAPULT_LOG(debug) << "verify failed due to timeout";
				});

				VerifiedPeerInfo serverPeerInfo{ node.identityKey(), m_settings.OutgoingSecurityMode };
				VerifyServer(connectedSocketInfo.socket(), serverPeerInfo, m_keyPair, [pThis = shared_from_this(), socketInfo, pRequest, node](
						auto verifyResult,
						const auto& verifiedPeerInfo) {
					if (VerifyResult::Success != verifyResult) {
						CATAPULT_LOG(warning) << "VerifyServer failed with " << verifyResult;
						if (VerifyResult::Failure_Challenge == verifyResult)
							CATAPULT_LOG(warning) << "failed verify signature of " << node << " with pubkey " << verifiedPeerInfo.PublicKey;

						return pRequest->callback(PeerConnectCode::Verify_Error, ionet::SslPacketSocketInfo());
					}

					return pRequest->callback(PeerConnectCode::Accepted, socketInfo);
				});
			}

		public:
			void shutdown() override {
				CATAPULT_LOG(info) << "closing all connections in SslServerConnector" << m_tag;
				m_sockets.clear();
			}

		private:
			boost::asio::io_context& m_ioContext;
			const crypto::KeyPair& m_keyPair;
			ConnectionSettings m_settings;

			std::string m_name;
			std::string m_tag;

			utils::WeakContainer<ionet::SslPacketSocket> m_sockets;
		};
	}

	std::shared_ptr<SslServerConnector> CreateSslServerConnector(
			thread::IoThreadPool& pool,
			const crypto::KeyPair& keyPair,
			const ConnectionSettings& settings,
			const char* name) {
		return std::make_shared<DefaultSslServerConnector>(pool, keyPair, settings, name ? std::string(name) : std::string());
	}
}}
