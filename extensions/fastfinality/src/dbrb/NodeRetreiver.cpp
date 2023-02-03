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

namespace catapult { namespace dbrb {

	namespace {
		void BroadcastNodes(const std::vector<SignedNode>& nodes, const net::PacketIoPickerContainer& packetIoPickers) {
			if (nodes.empty())
				return;

			auto timeout = utils::TimeSpan::FromSeconds(60);

			auto packetIoPairs = packetIoPickers.pickMultiple(timeout);
			CATAPULT_LOG(debug) << "found " << packetIoPairs.size() << " peer(s) for pushing DBRB nodes";
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

	void NodeRetreiver::enqueue(std::set<ProcessId> ids) {
		{
			std::lock_guard<std::mutex> guard(m_mutex);
			for (const auto& id : ids)
				m_buffer.emplace_back(id);
		}
		m_condVar.notify_one();
	}

	void NodeRetreiver::addNodes(const std::vector<SignedNode>& nodes) {
		std::vector<SignedNode> newNodes;

		{
			std::lock_guard<std::mutex> guard(m_mutex);
			for (const auto& node : nodes) {
				const ProcessId& id = node.Node.identityKey();
				auto iter = m_nodes.find(id);
				if (iter != m_nodes.end() && iter->second.Signature == node.Signature)
					continue;

				newNodes.emplace_back(node);
				m_nodes[id] = node;
			}
		}

		BroadcastNodes(newNodes, m_packetIoPickers);
	}

	std::optional<SignedNode> NodeRetreiver::getNode(const ProcessId& id) const {
		std::lock_guard<std::mutex> guard(m_mutex);
		std::optional<SignedNode> node;
		auto iter = m_nodes.find(id);
		if (iter != m_nodes.end())
			node = iter->second;

		return node;
	}

	void NodeRetreiver::processBuffer(BufferType& buffer) {
		std::set<ProcessId> ids;
		{
			std::lock_guard<std::mutex> guard(m_mutex);
			for (const auto& id : buffer) {
				auto iter = m_nodes.find(id);
				if (iter == m_nodes.end())
					ids.emplace(id);
			}
		}

		if (ids.empty())
			return;

		auto timeout = utils::TimeSpan::FromSeconds(60);

		auto packetIoPairs = m_packetIoPickers.pickMultiple(timeout);
		CATAPULT_LOG(debug) << "found " << packetIoPairs.size() << " peer(s) for pulling DBRB nodes";
		if (packetIoPairs.empty())
			return;

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
}}