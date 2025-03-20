/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "PacketReadersWriters.h"
#include "SslClientConnector.h"
#include "SslServerConnector.h"
#include "ClientConnector.h"
#include "ServerConnector.h"
#include "ChainedSocketReader.h"
#include "catapult/crypto/KeyPair.h"
#include "catapult/extensions/ServiceState.h"
#include "catapult/ionet/SocketReader.h"
#include "catapult/thread/IoThreadPool.h"
#include "catapult/utils/ModificationSafeIterableContainer.h"

namespace catapult { namespace net {

	namespace {
		enum class ConnectionType { Connected, Accepted };

		template<typename TPacketSocket>
		struct ReaderWriterState {
			net::ConnectionType ConnectionType;
			ionet::Node Node;
			std::weak_ptr<TPacketSocket> pSocketWeak;
			std::shared_ptr<ionet::PacketIo> pBufferedIo;
			std::weak_ptr<ChainedSocketReader> pReader;
		};

		// expected sequences
		// accept : insert -> remove
		// connect: prepareConnect -> abortConnect
		// connect: prepareConnect -> insert -> remove
		template<typename TPacketSocket>
		class WriterContainer {
		private:
			using ReaderWriterContainer = std::unordered_map<Key, ReaderWriterState<TPacketSocket>, utils::ArrayHasher<Key>>;
			using ChainedSocketReaderFactory = std::function<std::shared_ptr<ChainedSocketReader> (const std::shared_ptr<TPacketSocket>&, const std::shared_ptr<ionet::PacketIo>&)>;

		public:
			size_t size() const {
				std::shared_lock guard(m_mutex);
				return m_connectedWriters.size() + m_acceptedWriters.size();
			}

			size_t numOutgoingConnections() const {
				std::shared_lock guard(m_mutex);
				return m_connectedWriters.size() + m_acceptedWriters.size() + m_connectingNodeIdentityKeys.size();
			}

			utils::KeySet identities() const {
				std::shared_lock guard(m_mutex);
				return m_nodeIdentityKeys;
			}

			utils::KeySet peers() const {
				std::shared_lock guard(m_mutex);
				auto peers = m_nodeIdentityKeys;
				peers.insert(m_connectingNodeIdentityKeys.cbegin(), m_connectingNodeIdentityKeys.cend());
				return peers;
			}

			bool pickOne(ReaderWriterState<TPacketSocket>& state, const Key& identityKey) {
				std::shared_lock guard(m_mutex);
				auto iter = m_connectedWriters.find(identityKey);
				if (iter != m_connectedWriters.end()) {
					state = iter->second;
					return true;
				}

				iter = m_acceptedWriters.find(identityKey);
				if (iter != m_acceptedWriters.end()) {
					state = iter->second;
					return true;
				}

				return false;
			}

			bool prepareConnect(const ionet::Node& node) {
				std::unique_lock guard(m_mutex);
				const auto& identityKey = node.identityKey();
				if (!m_connectingNodeIdentityKeys.insert(identityKey).second ||
					m_connectedNodeIdentityKeys.find(identityKey) != m_connectedNodeIdentityKeys.cend()) {
					CATAPULT_LOG(debug) << "bypassing connection to already connected peer " << node;
					return false;
				}

				return true;
			}

			void abortConnect(const ionet::Node& node) {
				std::unique_lock guard(m_mutex);
				CATAPULT_LOG(debug) << "aborting connection to: " << node;
				m_connectingNodeIdentityKeys.erase(node.identityKey());
			}

			bool insert(net::ConnectionType connectionType, ionet::Node node, const std::shared_ptr<TPacketSocket>& pSocket, const ChainedSocketReaderFactory& readerFactory) {
				std::unique_lock guard(m_mutex);

				const auto& identityKey = node.identityKey();
				ReaderWriterState<TPacketSocket>* pState = nullptr;
				switch (connectionType) {
					case ConnectionType::Connected: {
						if (!m_connectedNodeIdentityKeys.emplace(identityKey).second) {
							CATAPULT_LOG(debug) << "ignoring connection to already connected peer " << node << " " << identityKey;
							return false;
						}
						m_connectingNodeIdentityKeys.erase(identityKey);
						pState = &m_connectedWriters[identityKey];
						break;
					}
					case ConnectionType::Accepted: {
						if (!m_acceptedNodeIdentityKeys.emplace(identityKey).second) {
							CATAPULT_LOG(debug) << "not accepting already connected peer " << node << " " << identityKey;
							return false;
						}
						pState = &m_acceptedWriters[identityKey];
						break;
					}
				}

				m_nodeIdentityKeys.emplace(identityKey);

				pState->ConnectionType = connectionType;
				pState->Node = node;
				pState->pSocketWeak = pSocket;
				pState->pBufferedIo = pSocket->buffered();
				// the reader takes ownership of the socket
				auto pReader = readerFactory(pSocket, pState->pBufferedIo);
				pReader->start();
				pState->pReader = pReader;

				return true;
			}

			void remove(const Key& identityKey, ConnectionType connectionType) {
				std::unique_lock guard(m_mutex);
				switch (connectionType) {
					case ConnectionType::Connected: {
						m_connectedNodeIdentityKeys.erase(identityKey);
						if (m_acceptedNodeIdentityKeys.find(identityKey) == m_acceptedNodeIdentityKeys.cend())
							m_nodeIdentityKeys.erase(identityKey);
						m_connectedWriters.erase(identityKey);
						break;
					}
					case ConnectionType::Accepted: {
						CATAPULT_LOG(debug) << "removing accepted connection to " << identityKey;
						m_acceptedNodeIdentityKeys.erase(identityKey);
						if (m_connectedNodeIdentityKeys.find(identityKey) == m_connectedNodeIdentityKeys.cend())
							m_nodeIdentityKeys.erase(identityKey);
						m_acceptedWriters.erase(identityKey);
						break;
					}
				}
			}

			void remove(const Key& identityKey) {
				std::unique_lock guard(m_mutex);
				m_nodeIdentityKeys.erase(identityKey);
				m_connectedNodeIdentityKeys.erase(identityKey);
				m_acceptedNodeIdentityKeys.erase(identityKey);
				m_connectedWriters.erase(identityKey);
				m_acceptedWriters.erase(identityKey);
			}

			void clear(bool includeConnecting) {
				CATAPULT_LOG(debug) << "clearing packet readers/writers (only established connections: " << (includeConnecting ? "false" : "true") << ")";
				std::unique_lock guard(m_mutex);
				m_nodeIdentityKeys.clear();
				if (includeConnecting)
					m_connectingNodeIdentityKeys.clear();
				m_connectedNodeIdentityKeys.clear();
				m_acceptedNodeIdentityKeys.clear();
				m_connectedWriters.clear();
				m_acceptedWriters.clear();
			}

		private:
			utils::KeySet m_nodeIdentityKeys; // keys of active writers (both connected AND accepted)
			utils::KeySet m_connectingNodeIdentityKeys;
			utils::KeySet m_connectedNodeIdentityKeys;
			utils::KeySet m_acceptedNodeIdentityKeys;
			ReaderWriterContainer m_connectedWriters;
			ReaderWriterContainer m_acceptedWriters;
			mutable std::shared_mutex m_mutex;
		};

		class ErrorHandlingPacketIo : public ionet::PacketIo {
		public:
			using ErrorCallback = action;

		public:
			ErrorHandlingPacketIo(
					std::shared_ptr<ionet::PacketIo> pPacketIo,
					ErrorCallback errorCallback)
					: m_pPacketIo(std::move(pPacketIo))
					, m_errorCallback(std::move(errorCallback))
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

		template<typename TPacketSocket, typename TClientConnector, typename TServerConnector>
		class BasePacketReadersWriters : public PacketReadersWriters {
		private:
			using SocketPointer = std::shared_ptr<TPacketSocket>;
			using WeakSocketPointer = std::weak_ptr<TPacketSocket>;

		public:
			BasePacketReadersWriters(
				const std::shared_ptr<thread::IoThreadPool>& pPool,
				const ionet::ServerPacketHandlers& handlers,
				extensions::ServiceState& state)
					: m_pPool(pPool)
					, m_handlers(handlers)
					, m_state(state)
			{}

		public:
			size_t numActiveConnections() const override {
				return m_writers.numOutgoingConnections();
			}

			utils::KeySet identities() const override {
				return m_writers.identities();
			}

			utils::KeySet peers() const override {
				return m_writers.peers();
			}

			void write(const Key& identityKey, const ionet::PacketPayload& payload, const WriteCallback& callback) override {
				ReaderWriterState<TPacketSocket> state;
				if (!m_writers.pickOne(state, identityKey)) {
					CATAPULT_LOG(debug) << "no packet io available for sending " << payload.header() << " to " << identityKey;
					callback(ionet::SocketOperationCode::Not_Connected);
					return;
				}

				auto errorHandler = [pThis = get_shared_from_this(), pSocketWeak = state.pSocketWeak, node = state.Node, connectionType = state.ConnectionType]() {
					CATAPULT_LOG(warning) << "error handler triggered for " << node << " " << node.identityKey();
					pThis->removeWriter(pSocketWeak, node.identityKey(), connectionType);
				};

				auto pPacketIo = std::make_shared<ErrorHandlingPacketIo>(state.pBufferedIo, errorHandler);
				CATAPULT_LOG(trace) << "checked out an io for sending " << payload.header() << " to " << identityKey;

				pPacketIo->write(payload, callback);
			}

			void closeActiveConnections() override {
				m_writers.clear(false);
			}

			bool closeOne(const Key& identityKey) override {
				m_writers.remove(identityKey);
			}

			void shutdown() override {
				CATAPULT_LOG(info) << "closing all connections in PacketReadersWriters";
				m_pClientConnector->shutdown();
				m_pServerConnector->shutdown();
				m_writers.clear(true);
			}

		protected:
			bool addWriter(const Key& key, const SocketPointer& pSocket, const std::string& host, ConnectionType connectionType) {
				auto node = ionet::Node(key, ionet::NodeEndpoint(), ionet::NodeMetadata(m_state.networkIdentifier()));
				return addWriter(node, pSocket, host, connectionType);
			}

			bool addWriter(const ionet::Node& node, const SocketPointer& pSocket, const std::string& host, ConnectionType connectionType) {
				auto identity = ionet::ReaderIdentity{ node.identityKey(), host };
				return m_writers.insert(connectionType, node, pSocket, [this, identity, connectionType](const auto& pSocket, const auto& pBufferedIo) {
					return this->createReader(pSocket, pBufferedIo, identity, connectionType);
				});
			}

			void removeWriter(const WeakSocketPointer& pSocketWeak, const Key& identityKey, ConnectionType connectionType) {
				auto pSocket = pSocketWeak.lock();
				if (pSocket)
					pSocket->close();

				m_writers.remove(identityKey, connectionType);
			}

			std::shared_ptr<ChainedSocketReader> createReader(
					const SocketPointer& pSocket,
					const std::shared_ptr<ionet::PacketIo>& pBufferedIo,
					const ionet::ReaderIdentity& identity,
					ConnectionType connectionType) {
				WeakSocketPointer pSocketWeak = pSocket;
				return CreateChainedSocketReader(pSocket, pBufferedIo, m_handlers, identity, [pThis = get_shared_from_this(), pSocketWeak, identityKey = identity.PublicKey, connectionType](auto code) {
					pThis->removeWriter(pSocketWeak, identityKey, connectionType);
				});
			}

			virtual std::shared_ptr<BasePacketReadersWriters> get_shared_from_this() = 0;

		protected:
			std::shared_ptr<thread::IoThreadPool> m_pPool;
			const ionet::ServerPacketHandlers& m_handlers;
			std::shared_ptr<TClientConnector> m_pClientConnector;
			std::shared_ptr<TServerConnector> m_pServerConnector;
			extensions::ServiceState& m_state;
			WriterContainer<TPacketSocket> m_writers;
		};

		class SslPacketReadersWriters
			: public BasePacketReadersWriters<ionet::SslPacketSocket, SslClientConnector, SslServerConnector>
			, public std::enable_shared_from_this<SslPacketReadersWriters> {
		public:
			SslPacketReadersWriters(
				const std::shared_ptr<thread::IoThreadPool>& pPool,
				const ionet::ServerPacketHandlers& handlers,
				const crypto::KeyPair& keyPair,
				const ConnectionSettings& settings,
				extensions::ServiceState& state)
					: BasePacketReadersWriters(pPool, handlers, state) {
				m_pClientConnector = CreateSslClientConnector(*m_pPool, keyPair, settings);
				m_pServerConnector = CreateSslServerConnector(*m_pPool, keyPair, settings);
			}

		public:
			void connect(const ionet::Node& node, const ConnectCallback& callback) override {
				if (!m_writers.prepareConnect(node))
					return callback(PeerConnectCode::Already_Connected);

				m_pServerConnector->connect(node, [pThis = shared_from_this(), node, callback](
						auto connectCode,
						const auto& verifiedSocketInfo) {
					// abort the connection if it failed or is redundant
					if (PeerConnectCode::Accepted != connectCode || !pThis->addWriter(node, verifiedSocketInfo.socket(), verifiedSocketInfo.host(), ConnectionType::Connected)) {
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
					if (PeerConnectCode::Accepted == connectCode) {
						if (!pThis->addWriter(remoteKey, pVerifiedSocket, host, ConnectionType::Accepted)) {
							connectCode = PeerConnectCode::Already_Connected;
						} else {
							CATAPULT_LOG(debug) << "accepted connection from '" << host << "' as " << remoteKey;
						}
					}

					callback({ connectCode, remoteKey });
				});
			}

			void accept(const ionet::AcceptedPacketSocketInfo& socketInfo, const ConnectCallback& callback) override {
				CATAPULT_LOG(warning) << "rejecting insecure connection from " << socketInfo.host();
				callback({ PeerConnectCode::Invalid_Socket_Type, Key() });
			}

		protected:
			std::shared_ptr<BasePacketReadersWriters> get_shared_from_this() override {
				return shared_from_this();
			}
		};

		class DefaultPacketReadersWriters
			: public BasePacketReadersWriters<ionet::PacketSocket, ClientConnector, ServerConnector>
			, public std::enable_shared_from_this<DefaultPacketReadersWriters> {
		public:
			DefaultPacketReadersWriters(
				const std::shared_ptr<thread::IoThreadPool>& pPool,
				const ionet::ServerPacketHandlers& handlers,
				const crypto::KeyPair& keyPair,
				const ConnectionSettings& settings,
				extensions::ServiceState& state)
					: BasePacketReadersWriters(pPool, handlers, state) {
				m_pClientConnector = CreateClientConnector(m_pPool, keyPair, settings);
				m_pServerConnector = CreateServerConnector(m_pPool, keyPair, settings);
			}

		public:
			void connect(const ionet::Node& node, const ConnectCallback& callback) override {
				if (!m_writers.prepareConnect(node))
					return callback(PeerConnectCode::Already_Connected);

				m_pServerConnector->connect(node, [pThis = shared_from_this(), node, callback](
						auto connectCode,
						const auto& pVerifiedSocket) {
					// abort the connection if it failed or is redundant
					if (PeerConnectCode::Accepted != connectCode || !pThis->addWriter(node, pVerifiedSocket, node.endpoint().Host, ConnectionType::Connected)) {
						pThis->m_writers.abortConnect(node);

						if (PeerConnectCode::Accepted == connectCode)
							connectCode = PeerConnectCode::Already_Connected;
					}

					callback({ connectCode, node.identityKey() });
				});
			}

			void accept(const ionet::SslPacketSocketInfo& socketInfo, const ConnectCallback& callback) override {
				CATAPULT_LOG(warning) << "rejecting secure connection from " << socketInfo.host() << " : " << socketInfo.publicKey();
				callback({ PeerConnectCode::Invalid_Socket_Type, socketInfo.publicKey() });
			}

			void accept(const ionet::AcceptedPacketSocketInfo& socketInfo, const ConnectCallback& callback) override {
				m_pClientConnector->accept(socketInfo.socket(), [pThis = shared_from_this(), host = socketInfo.host(), callback](
						auto connectCode,
						const auto& pVerifiedSocket,
						const auto& remoteKey) {
					if (PeerConnectCode::Accepted == connectCode) {
						if (!pThis->addWriter(remoteKey, pVerifiedSocket, host, ConnectionType::Accepted)) {
							connectCode = PeerConnectCode::Already_Connected;
						} else {
							CATAPULT_LOG(debug) << "accepted connection from '" << host << "' as " << remoteKey;
						}
					}

					callback({ connectCode, remoteKey });
				});
			}

		protected:
			std::shared_ptr<BasePacketReadersWriters> get_shared_from_this() override {
				return shared_from_this();
			}
		};
	}

	std::shared_ptr<PacketReadersWriters> CreatePacketReadersWriters(
			const std::shared_ptr<thread::IoThreadPool>& pPool,
			const ionet::ServerPacketHandlers& handlers,
			const crypto::KeyPair& keyPair,
			const ConnectionSettings& settings,
			extensions::ServiceState& state,
			bool secure) {
		if (secure) {
			return std::make_shared<SslPacketReadersWriters>(pPool, handlers, keyPair, settings, state);
		} else {
			return std::make_shared<DefaultPacketReadersWriters>(pPool, handlers, keyPair, settings, state);
		}
	}
}}
