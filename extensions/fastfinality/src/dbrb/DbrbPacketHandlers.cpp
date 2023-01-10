/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "DbrbPacketHandlers.h"
#include "DbrbChainPackets.h"
#include "DbrbProcess.h"
#include "catapult/ionet/PacketPayloadFactory.h"

namespace catapult { namespace dbrb {

	void RegisterPushNodesHandler(
			const std::weak_ptr<DbrbProcess>& pDbrbProcessWeak,
			model::NetworkIdentifier networkIdentifier,
			ionet::ServerPacketHandlers& handlers) {
		handlers.registerHandler(ionet::PacketType::Dbrb_Push_Nodes, [pDbrbProcessWeak, networkIdentifier](const auto& packet, auto&) {
			const auto pPacket = ionet::CoercePacket<DbrbPushNodesPacket>(&packet);
			if (!pPacket) {
				CATAPULT_LOG(warning) << "rejecting empty request: " << packet;
				return;
			}

			CATAPULT_LOG(trace) << "received valid " << packet;

			auto pDbrbProcessShared = pDbrbProcessWeak.lock();
			if (!pDbrbProcessShared)
				return;

			std::vector<SignedNode> nodes = ReadNodesFromPacket<DbrbPushNodesPacket>(packet, networkIdentifier);
			pDbrbProcessShared->nodeRetreiver().addNodes(nodes);
		});
	}

	void RegisterPullNodesHandler(
			const std::weak_ptr<DbrbProcess>& pDbrbProcessWeak,
			ionet::ServerPacketHandlers& handlers) {
		handlers.registerHandler(ionet::PacketType::Dbrb_Pull_Nodes, [pDbrbProcessWeak](const auto& packet, auto& context) {
			const auto pRequest = ionet::CoercePacket<DbrbPullNodesPacket>(&packet);
			if (!pRequest) {
				CATAPULT_LOG(warning) << "rejecting empty request: " << packet;
				return;
			}

			CATAPULT_LOG(trace) << "received valid " << packet;

			auto pDbrbProcessShared = pDbrbProcessWeak.lock();
			if (!pDbrbProcessShared)
				return;

			const auto& nodeRetreiver = pDbrbProcessShared->nodeRetreiver();
			std::vector<std::pair<model::UniqueEntityPtr<ionet::NetworkNode>, Signature>> networkNodes;
			networkNodes.reserve(pRequest->ProcessIdCount);
			uint32_t payloadSize = 0u;
			const auto* pIdBuffer = reinterpret_cast<const uint8_t*>(pRequest + 1);
			for (auto i = 0u; i < pRequest->ProcessIdCount; ++i) {
				ProcessId id;
				memcpy(id.data(), pIdBuffer, ProcessId_Size);
				pIdBuffer += ProcessId_Size;

				auto node = nodeRetreiver.getNode(id);
				if (node) {
					networkNodes.emplace_back(ionet::PackNode(node.value().Node), node.value().Signature);
					payloadSize += networkNodes.back().first->Size;
				}
			}

			auto pResponse = ionet::CreateSharedPacket<DbrbPullNodesPacket>(payloadSize);
			pResponse->ProcessIdCount = 0u;
			pResponse->NodeCount = utils::checked_cast<size_t, uint16_t>(networkNodes.size());
			auto* pBuffer = reinterpret_cast<uint8_t*>(pResponse.get() + 1);
			for (const auto& [pNetworkNode, signature] : networkNodes) {
				memcpy(pBuffer, pNetworkNode.get(), pNetworkNode->Size);
				pBuffer += pNetworkNode->Size;
				memcpy(pBuffer, signature.data(), Signature_Size);
				pBuffer += Signature_Size;
			}

			context.response(ionet::PacketPayload(pResponse));
		});
	}
}}