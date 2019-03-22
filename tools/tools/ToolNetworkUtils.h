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
#include "catapult/crypto/KeyPair.h"
#include "catapult/thread/Future.h"
#include "catapult/types.h"

namespace catapult {
	namespace ionet {
		class Node;
		class PacketIo;
	}
	namespace thread { class IoThreadPool; }
}

namespace catapult { namespace tools {

	/// Future that returns a packet io shared pointer.
	using PacketIoFuture = thread::future<std::shared_ptr<ionet::PacketIo>>;

	/// Connects to localhost as a client with \a clientKeyPair using \a pPool.
	/// Localhost is expected to have identity \a serverPublicKey.
	PacketIoFuture ConnectToLocalNode(
			const crypto::KeyPair& clientKeyPair,
			const Key& serverPublicKey,
			const std::shared_ptr<thread::IoThreadPool>& pPool);

	/// Connects to \a node as a client with \a clientKeyPair using \a pPool.
	PacketIoFuture ConnectToNode(
			const crypto::KeyPair& clientKeyPair,
			const ionet::Node& node,
			const std::shared_ptr<thread::IoThreadPool>& pPool);

	/// Helper class for connecting to multiple nodes.
	class MultiNodeConnector {
	public:
		/// Creates a connector.
		MultiNodeConnector();

		/// Destroys the connector.
		~MultiNodeConnector();

	public:
		/// Gets the underlying pool used by the connector.
		thread::IoThreadPool& pool();

	public:
		/// Connects to \a node.
		PacketIoFuture connect(const ionet::Node& node);

	private:
		crypto::KeyPair m_clientKeyPair;
		std::shared_ptr<thread::IoThreadPool> m_pPool;
	};
}}
