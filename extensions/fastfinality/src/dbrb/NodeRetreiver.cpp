/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "NodeRetreiver.h"
#include "DbrbChainPackets.h"
#include "catapult/api/RemoteRequestDispatcher.h"
#include "catapult/net/PacketIoPickerContainer.h"
#include "catapult/utils/TimeSpan.h"
#include <thread>

namespace catapult { namespace dbrb {

	namespace {
		void BroadcastNodes(const std::vector<SignedNode>& nodes, const net::PacketIoPickerContainer& packetIoPickers) {
			if (nodes.empty())
				return;

			auto packetIoPairs = packetIoPickers.pickMultiple(utils::TimeSpan::FromSeconds(60));
			CATAPULT_LOG(debug) << "found " << packetIoPairs.size() << " peer(s) for pushing " << nodes.size() << " DBRB node(s)";
			if (packetIoPairs.empty())
				return;

			auto nodeCount = utils::checked_cast<size_t, uint16_t>(nodes.size());

			std::vector<std::pair<model::UniqueEntityPtr<ionet::NetworkNode>, Signature>> networkNodes;
			networkNodes.reserve(nodeCount);
			auto payloadSize = utils::checked_cast<size_t, uint32_t>(nodeCount * Signature_Size);
			for (const auto& node : nodes) {
				networkNodes.emplace_back(ionet::PackNode(node.Node), node.Signature);
				payloadSize += networkNodes.back().first->Size;
			}

			auto pPacket = ionet::CreateSharedPacket<DbrbPushNodesPacket>(payloadSize);
			pPacket->NodeCount = nodeCount;
			auto* pBuffer = reinterpret_cast<uint8_t*>(pPacket.get() + 1);
			for (const auto& [pNetworkNode, signature] : networkNodes) {
				memcpy(pBuffer, pNetworkNode.get(), pNetworkNode->Size);
				pBuffer += pNetworkNode->Size;
				memcpy(pBuffer, signature.data(), Signature_Size);
				pBuffer += Signature_Size;
			}

			for (const auto& packetIoPair : packetIoPairs) {
				auto pPacketIoPair = std::make_shared<ionet::NodePacketIoPair>(packetIoPair);
				pPacketIoPair->io()->write(ionet::PacketPayload(pPacket), [pPacket, pPacketIoPair](ionet::SocketOperationCode code) {
					if (code != ionet::SocketOperationCode::Success)
						CATAPULT_LOG(warning) << "[DBRB] sending " << *pPacket << " to " << pPacketIoPair->node() << " completed with " << code;
				});
			}
		}
	}

	NodeRetreiver::NodeRetreiver(const net::PacketIoPickerContainer& packetIoPickers, model::NetworkIdentifier networkIdentifier)
		: m_packetIoPickers(packetIoPickers)
		, m_networkIdentifier(networkIdentifier)
	{}

	void NodeRetreiver::requestNodes(const std::set<ProcessId>& requestedIds) {
		std::set<ProcessId> ids;

		{
			std::lock_guard<std::mutex> guard(m_mutex);
			for (const auto& id : requestedIds) {
				if (m_nodes.find(id) == m_nodes.end())
					ids.emplace(id);
			}
		}

		if (ids.empty())
			return;

		auto packetIoPairs = m_packetIoPickers.pickMultiple(utils::TimeSpan::FromSeconds(60));
		CATAPULT_LOG(debug) << "found " << packetIoPairs.size() << " peer(s) for pulling " << ids.size() << " DBRB node(s)";
		if (!packetIoPairs.empty()) {
			std::vector<SignedNode> newNodes;
			for (const auto& packetIoPair : packetIoPairs) {
				api::RemoteRequestDispatcher dispatcher(*packetIoPair.io());
				std::vector<SignedNode> nodes;
				try {
					nodes = dispatcher.dispatch(DbrbPullNodesTraits(m_networkIdentifier), ids).get();
				} catch (const std::exception& error) {
					CATAPULT_LOG(error) << error.what();
				}

				for (const auto& node : nodes) {
					newNodes.emplace_back(node);
					ids.erase(node.Node.identityKey());
				}

				if (ids.empty())
					break;
			}

			addNodes(newNodes);
		}
	}

	void NodeRetreiver::addNodes(const std::vector<SignedNode>& nodes) {
		std::lock_guard<std::mutex> guard(m_mutex);
		for (const auto& node : nodes) {
			const auto& id = node.Node.identityKey();
			auto iter = m_nodes.find(id);
			if (iter != m_nodes.end() && iter->second.Signature == node.Signature)
				continue;

			m_nodes[id] = node;
		}
	}

	void NodeRetreiver::broadcastNodes() const {
		std::vector<SignedNode> nodes;

		{
			std::lock_guard<std::mutex> guard(m_mutex);
			nodes.reserve(m_nodes.size());
			for (const auto& [_, node] : m_nodes)
				nodes.emplace_back(node);
		}

		BroadcastNodes(nodes, m_packetIoPickers);
	}

	std::optional<SignedNode> NodeRetreiver::getNode(const ProcessId& id) const {
		std::optional<SignedNode> node;

		{
			std::lock_guard<std::mutex> guard(m_mutex);
			auto iter = m_nodes.find(id);
			if (iter != m_nodes.end())
				node = iter->second;
		}

		return node;
	}
}}