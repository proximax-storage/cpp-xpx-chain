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
		class SslPacketSocket;
		class SslPacketSocketInfo;
	}
	namespace thread { class IoThreadPool; }
}

namespace catapult { namespace net {

	/// Accepts connections that are initiated by external nodes to this (local) node.
	class SslClientConnector {
	public:
		/// Callback that is passed the accept result as well as the client socket and public key (on success).
		using SslAcceptCallback = consumer<PeerConnectCode, const std::shared_ptr<ionet::SslPacketSocket>&, const Key&>;

	public:
		virtual ~SslClientConnector() = default;

	public:
		/// Gets the number of active connections.
		virtual size_t numActiveConnections() const = 0;

		/// Gets the friendly name of this connector.
		virtual const std::string& name() const = 0;

	public:
		/// Accepts a connection represented by \a acceptedSocketInfo and calls \a callback on completion.
		virtual void accept(const ionet::SslPacketSocketInfo& acceptedSocketInfo, const SslAcceptCallback& callback) = 0;

		/// Shuts down all connections.
		virtual void shutdown() = 0;
	};

	/// Creates a client connector for a server with specified \a serverPublicKey using \a pool and configured with \a settings.
	/// Optional friendly \a name can be provided to tag logs.
	std::shared_ptr<SslClientConnector> CreateSslClientConnector(
			thread::IoThreadPool& pool,
			const crypto::KeyPair& keyPair,
			const ConnectionSettings& settings,
			const char* name = nullptr);
}}
