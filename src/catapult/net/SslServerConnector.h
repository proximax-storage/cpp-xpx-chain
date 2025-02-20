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

#pragma once
#include "ConnectionSettings.h"
#include "PeerConnectCode.h"
#include "catapult/functions.h"
#include <memory>

namespace catapult {
	namespace crypto { class KeyPair; }
	namespace ionet {
		class Node;
		class PacketSocket;
		class SslPacketSocketInfo;
	}
	namespace thread { class IoThreadPool; }
}

namespace catapult { namespace net {

	/// Establishes connections with external nodes that this (local) node initiates.
	class SslServerConnector {
	public:
		/// Callback that is passed the connect result and the connected socket info on success.
		using SslConnectCallback = consumer<PeerConnectCode, const ionet::SslPacketSocketInfo&>;

	public:
		virtual ~SslServerConnector() = default;

	public:
		/// Gets the number of active connections.
		virtual size_t numActiveConnections() const = 0;

		/// Gets the friendly name of this connector.
		virtual const std::string& name() const = 0;

	public:
		/// Attempts to connect to \a node and calls \a callback on completion.
		virtual void connect(const ionet::Node& node, const SslConnectCallback& callback) = 0;

		/// Shuts down all connections.
		virtual void shutdown() = 0;
	};

	/// Creates a server connector for a server with specified \a serverPublicKey using \a pool and configured with \a settings.
	/// Optional friendly \a name can be provided to tag logs.
	std::shared_ptr<SslServerConnector> CreateSslServerConnector(
			thread::IoThreadPool& pool,
			const crypto::KeyPair& keyPair,
			const ConnectionSettings& settings,
			const char* name = nullptr);
}}
