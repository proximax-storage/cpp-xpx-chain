/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "DbrbPacketHandlers.h"
#include "DbrbChainPackets.h"
#include "DbrbProcess.h"
#include "ShardedDbrbProcess.h"
#include "catapult/crypto/Signer.h"
#include "catapult/ionet/NetworkNode.h"
#include "catapult/utils/NetworkTime.h"

namespace catapult { namespace dbrb {

	namespace {
		constexpr auto MAX_DELAY = Timestamp(15000);

		template<typename TDbrbProcess>
		void RegisterPushNodesHandlerImpl(
				const std::weak_ptr<TDbrbProcess>& pDbrbProcessWeak,
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
				if (nodes.empty())
					return;

				pDbrbProcessShared->messageSender()->addNodes(nodes);
			});
		}

		template<typename TDbrbProcess>
		void RegisterPullNodesHandlerImpl(
				const std::weak_ptr<TDbrbProcess>& pDbrbProcessWeak,
				ionet::ServerPacketHandlers& handlers) {
			handlers.registerHandler(ionet::PacketType::Dbrb_Pull_Nodes, [pDbrbProcessWeak](const ionet::Packet& packet, auto& context) {
				auto pDbrbProcessShared = pDbrbProcessWeak.lock();
				if (!pDbrbProcessShared)
					return;

				std::set<ProcessId> requestedIds;
				const auto* pPacket = reinterpret_cast<const DbrbPullNodesPacket*>(&packet);
				const auto* pProcessId = reinterpret_cast<const ProcessId*>(pPacket + 1);
				for (auto i = 0; i < pPacket->NodeCount; ++i, ++pProcessId)
					requestedIds.insert(*pProcessId);

				auto pMessageSender = pDbrbProcessShared->messageSender();
				auto nodes = pMessageSender->getKnownNodes(requestedIds);
				for (const auto& node : nodes)
					CATAPULT_LOG(trace) << "[MESSAGE SENDER] sharing node " << node << " [dbrb port " << node.endpoint().DbrbPort << "] " << node.identityKey();
				pMessageSender->sendNodes(nodes, context.key());
			});
		}

		template<typename TDbrbProcess>
		void RegisterRemoveNodeRequestHandlerImpl(
				const std::weak_ptr<TDbrbProcess>& pDbrbProcessWeak,
				const crypto::KeyPair& keyPair,
				ionet::ServerPacketHandlers& handlers) {
			handlers.registerHandler(ionet::PacketType::Dbrb_Remove_Node_Request, [pDbrbProcessWeak, &keyPair](const ionet::Packet& packet, auto& context) {
				// TODO: uncomment and retest
//				auto pDbrbProcessShared = pDbrbProcessWeak.lock();
//				if (!pDbrbProcessShared)
//					return;
//
//				const auto* pRequestPacket = reinterpret_cast<const DbrbRemoveNodeRequestPacket*>(&packet);
//				auto now = utils::NetworkTime();
//				if (pDbrbProcessShared->messageSender()->isNodeAdded(pRequestPacket->ProcessId) || now < pRequestPacket->Timestamp || now - pRequestPacket->Timestamp > MAX_DELAY)
//					return;
//
//				auto pResponsePacket = ionet::CreateSharedPacket<DbrbRemoveNodeResponsePacket>();
//				pResponsePacket->Timestamp = pRequestPacket->Timestamp;
//				pResponsePacket->ProcessId = pRequestPacket->ProcessId;
//				auto hash = CalculateHash({ { reinterpret_cast<const uint8_t*>(&pResponsePacket->Timestamp), sizeof(Timestamp) }, { pResponsePacket->ProcessId.data(), Key_Size } });
//				crypto::Sign(keyPair, hash, pResponsePacket->Signature);
//				pDbrbProcessShared->messageSender()->enqueue(pResponsePacket, { context.key() });
			});
		}

		template<typename TDbrbProcess>
		void RegisterRemoveNodeResponseHandlerImpl(
				const std::weak_ptr<TDbrbProcess>& pDbrbProcessWeak,
				ionet::ServerPacketHandlers& handlers) {
			handlers.registerHandler(ionet::PacketType::Dbrb_Remove_Node_Response, [pDbrbProcessWeak](const ionet::Packet& packet, auto& context) {
				// TODO: uncomment and retest
//				auto pDbrbProcessShared = pDbrbProcessWeak.lock();
//				if (!pDbrbProcessShared)
//					return;
//
//				const auto* pPacket = reinterpret_cast<const DbrbRemoveNodeResponsePacket*>(&packet);
//				auto hash = CalculateHash({ { reinterpret_cast<const uint8_t*>(&pPacket->Timestamp), sizeof(Timestamp) }, { pPacket->ProcessId.data(), Key_Size } });
//				if (crypto::Verify(context.key(), hash, pPacket->Signature))
//					pDbrbProcessShared->messageSender()->addRemoveNodeResponse(pPacket->ProcessId, context.key(), pPacket->Timestamp, pPacket->Signature);
			});
		}
	}

	void RegisterPushNodesHandler(
			const std::weak_ptr<DbrbProcess>& pDbrbProcessWeak,
			model::NetworkIdentifier networkIdentifier,
			ionet::ServerPacketHandlers& handlers) {
		RegisterPushNodesHandlerImpl(pDbrbProcessWeak, networkIdentifier, handlers);
	}

	void RegisterPushNodesHandler(
			const std::weak_ptr<ShardedDbrbProcess>& pDbrbProcessWeak,
			model::NetworkIdentifier networkIdentifier,
			ionet::ServerPacketHandlers& handlers) {
		RegisterPushNodesHandlerImpl(pDbrbProcessWeak, networkIdentifier, handlers);
	}

	void RegisterPullNodesHandler(
			const std::weak_ptr<DbrbProcess>& pDbrbProcessWeak,
			ionet::ServerPacketHandlers& handlers) {
		RegisterPullNodesHandlerImpl(pDbrbProcessWeak, handlers);
	}

	void RegisterPullNodesHandler(
			const std::weak_ptr<ShardedDbrbProcess>& pDbrbProcessWeak,
			ionet::ServerPacketHandlers& handlers) {
		RegisterPullNodesHandlerImpl(pDbrbProcessWeak, handlers);
	}

	void RegisterRemoveNodeRequestHandler(
			const std::weak_ptr<DbrbProcess>& pDbrbProcessWeak,
			const crypto::KeyPair& keyPair,
			ionet::ServerPacketHandlers& handlers) {
		RegisterRemoveNodeRequestHandlerImpl(pDbrbProcessWeak, keyPair, handlers);
	}

	void RegisterRemoveNodeRequestHandler(
			const std::weak_ptr<ShardedDbrbProcess>& pDbrbProcessWeak,
			const crypto::KeyPair& keyPair,
			ionet::ServerPacketHandlers& handlers) {
		RegisterRemoveNodeRequestHandlerImpl(pDbrbProcessWeak, keyPair, handlers);
	}

	void RegisterRemoveNodeResponseHandler(
			const std::weak_ptr<DbrbProcess>& pDbrbProcessWeak,
			ionet::ServerPacketHandlers& handlers) {
		RegisterRemoveNodeResponseHandlerImpl(pDbrbProcessWeak, handlers);
	}

	void RegisterRemoveNodeResponseHandler(
			const std::weak_ptr<ShardedDbrbProcess>& pDbrbProcessWeak,
			ionet::ServerPacketHandlers& handlers) {
		RegisterRemoveNodeResponseHandlerImpl(pDbrbProcessWeak, handlers);
	}
}}