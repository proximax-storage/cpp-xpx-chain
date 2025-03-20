/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "ConnectionContainer.h"
#include "ConnectionSettings.h"
#include "PacketIoPicker.h"
#include "PeerConnectResult.h"
#include "catapult/ionet/PacketHandlers.h"
#include "catapult/ionet/SslPacketSocket.h"
#include "catapult/ionet/PacketSocket.h"
#include <memory>

namespace catapult {
	namespace crypto { class KeyPair; }
	namespace extensions { class ServiceState; }
	namespace ionet { class Node; }
	namespace net { struct Packet; }
	namespace thread { class IoThreadPool; }
}

namespace catapult { namespace net {

	/// Manages a collection of connections that send data to and receive from external nodes.
	class PacketReadersWriters : public ConnectionContainer {
	public:
		using ConnectCallback = consumer<const PeerConnectResult&>;
		using WriteCallback = consumer<ionet::SocketOperationCode>;

	public:
		/// Attempts to connect to \a node and calls \a callback on completion.
		virtual void connect(const ionet::Node& node, const ConnectCallback& callback) = 0;

		/// Accepts a secure connection represented by \a socketInfo and calls \a callback on completion.
		virtual void accept(const ionet::SslPacketSocketInfo& socketInfo, const ConnectCallback& callback) = 0;

		/// Accepts an insecure connection represented by \a socketInfo and calls \a callback on completion.
		virtual void accept(const ionet::AcceptedPacketSocketInfo& socketInfo, const ConnectCallback& callback) = 0;

		/// Closes all active connections.
		virtual void closeActiveConnections() = 0;

		/// Gets the identities of active connections and connecting peers.
		virtual utils::KeySet peers() const = 0;

		/// Shuts down all connections.
		virtual void shutdown() = 0;

	public:
		/// Writes \a payload to the node identified by \a identityKey and calls \a callback on completion.
		virtual void write(const Key& identityKey, const ionet::PacketPayload& payload, const WriteCallback& callback) = 0;
	};

	std::shared_ptr<PacketReadersWriters> CreatePacketReadersWriters(
			const std::shared_ptr<thread::IoThreadPool>& pPool,
			const ionet::ServerPacketHandlers& handlers,
			const crypto::KeyPair& keyPair,
			const ConnectionSettings& settings,
			extensions::ServiceState& state,
			bool secure);
}}
