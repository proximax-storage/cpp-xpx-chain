/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "DbrbPacketHandlers.h"
#include "DbrbChainPackets.h"
#include "DbrbProcess.h"
#include "catapult/ionet/NetworkNode.h"

namespace catapult { namespace dbrb {

	void RegisterPushNodesHandler(
			const std::weak_ptr<DbrbProcess>& pDbrbProcessWeak,
			model::NetworkIdentifier networkIdentifier,
			ionet::ServerPacketHandlers& handlers) {
		handlers.registerHandler(ionet::PacketType::Dbrb_Push_Nodes, [pDbrbProcessWeak, networkIdentifier](const ionet::Packet& packet, auto& context) {
			auto pDbrbProcessShared = pDbrbProcessWeak.lock();
			if (!pDbrbProcessShared)
				return;

			std::vector<ionet::Node> nodes;
			const auto* pPacket = reinterpret_cast<const DbrbPushNodesPacket*>(&packet);
			const auto* pBuffer = reinterpret_cast<const uint8_t*>(pPacket + 1);
			for (auto i = 0; i < pPacket->NodeCount; ++i) {
				const auto* pNetworkNode = reinterpret_cast<const ionet::NetworkNode*>(pBuffer);
				pBuffer += pNetworkNode->Size;
				if (pNetworkNode->NetworkIdentifier != networkIdentifier)
					continue;
				nodes.emplace_back(ionet::UnpackNode(*pNetworkNode));
			}

			for (auto& node : nodes) {
				if (node.endpoint().Host.empty() && node.identityKey() == context.key()) {
					auto endpoint = ionet::NodeEndpoint{ context.host(), node.endpoint().Port };
					CATAPULT_LOG(debug) << "[DBRB] auto detected host '" << endpoint.Host << "' for " << node.identityKey();
					node = ionet::Node(node.identityKey(), endpoint, node.metadata());
					break;
				}
			}
			nodes.erase(std::remove_if(nodes.begin(), nodes.end(), [](const auto& node) { return node.endpoint().Host.empty(); }), nodes.end());
			pDbrbProcessShared->messageSender()->addNodes(nodes);

			auto pResponsePacket = ionet::CreateSharedPacket<DbrbPushNodesPacket>(packet.Size - sizeof(DbrbPushNodesPacket));
			memcpy(pResponsePacket.get(), &packet, packet.Size);
			pDbrbProcessShared->messageSender()->broadcastNodes(pResponsePacket);
		});
	}
}}