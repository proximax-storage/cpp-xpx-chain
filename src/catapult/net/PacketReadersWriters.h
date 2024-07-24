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
#include <memory>

namespace catapult {
	namespace crypto { class KeyPair; }
	namespace extensions { class ServiceState; }
	namespace ionet {
		class Node;
		class PacketSocket;
		class SslPacketSocketInfo;
	}
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

		/// Accepts a connection represented by \a socketInfo and calls \a callback on completion.
		virtual void accept(const ionet::SslPacketSocketInfo& socketInfo, const ConnectCallback& callback) = 0;

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
			extensions::ServiceState& state);
}}
