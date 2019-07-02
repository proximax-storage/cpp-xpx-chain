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
#include "catapult/config/LocalNodeConfiguration.h"
#include "catapult/ionet/Node.h"
#include "catapult/ionet/PacketHandlers.h"
#include "catapult/model/NetworkInfo.h"

namespace catapult { namespace ionet { struct NetworkNode; } }

namespace catapult { namespace handlers {

	/// Consumes a node.
	using NodeConsumer = consumer<const ionet::Node&>;

	/// Consumes multiple nodes.
	using NodesConsumer = consumer<const ionet::NodeSet&>;

	/// Supplies nodes.
	using NodesSupplier = supplier<ionet::NodeSet>;

	/// Registers a node discovery push ping handler in \a handlers that forwards node information to \a nodeConsumer given
	/// the current network identifier (\a networkIdentifier).
	void RegisterNodeDiscoveryPushPingHandler(
			ionet::ServerPacketHandlers& handlers,
			model::NetworkIdentifier networkIdentifier,
			const NodeConsumer& nodeConsumer);

	/// Registers a node discovery pull ping handler in \a handlers that responds with the local node created from \a config.
	/// \note shared_ptr is used to avoid copying into packet.
	void RegisterNodeDiscoveryPullPingHandler(
			ionet::ServerPacketHandlers& handlers,
			const config::LocalNodeConfiguration& config);

	/// Registers a node discovery push peers handler in \a handlers that forwards received nodes to \a nodesConsumer.
	void RegisterNodeDiscoveryPushPeersHandler(ionet::ServerPacketHandlers& handlers, const NodesConsumer& nodesConsumer);

	/// Registers a node discovery pull peers handler in \a handlers that responds with nodes from \a nodesSupplier.
	void RegisterNodeDiscoveryPullPeersHandler(ionet::ServerPacketHandlers& handlers, const NodesSupplier& nodesSupplier);
}}
