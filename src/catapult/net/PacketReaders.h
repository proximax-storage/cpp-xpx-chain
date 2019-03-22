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

#pragma once
#include "ConnectionContainer.h"
#include "ConnectionSettings.h"
#include "PeerConnectResult.h"
#include "catapult/ionet/PacketHandlers.h"
#include <functional>
#include <memory>

namespace catapult {
	namespace crypto { class KeyPair; }
	namespace ionet {
		class AcceptedPacketSocketInfo;
		class PacketIo;
		class PacketSocket;
	}
	namespace thread { class IoThreadPool; }
}

namespace catapult { namespace net {

	/// Manages a collection of connections that receive data from external nodes.
	class PacketReaders : public ConnectionContainer {
	public:
		using AcceptCallback = consumer<const PeerConnectResult&>;

	public:
		/// Gets the number of active readers.
		virtual size_t numActiveReaders() const = 0;

	public:
		/// Accepts a connection represented by \a socketInfo and calls \a callback on completion.
		virtual void accept(const ionet::AcceptedPacketSocketInfo& socketInfo, const AcceptCallback& callback) = 0;

		/// Shuts down all connections.
		virtual void shutdown() = 0;
	};

	/// Creates a packet readers container for a server with a key pair of \a keyPair using \a pPool and \a handlers,
	/// configured with \a settings and allowing \a maxConnectionsPerIdentity.
	std::shared_ptr<PacketReaders> CreatePacketReaders(
			const std::shared_ptr<thread::IoThreadPool>& pPool,
			const ionet::ServerPacketHandlers& handlers,
			const crypto::KeyPair& keyPair,
			const ConnectionSettings& settings,
			uint32_t maxConnectionsPerIdentity);
}}
