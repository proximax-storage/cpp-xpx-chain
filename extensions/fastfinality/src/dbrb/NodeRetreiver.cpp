/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "NodeRetreiver.h"
#include "catapult/ionet/NodeContainer.h"
#include "catapult/net/PacketWriters.h"
#include "catapult/utils/TimeSpan.h"
#include <thread>

namespace catapult { namespace dbrb {

	namespace {
		constexpr utils::LogLevel MapToLogLevel(net::PeerConnectCode connectCode) {
			if (connectCode == net::PeerConnectCode::Accepted)
				return utils::LogLevel::Debug;
			else
				return utils::LogLevel::Warning;
		}
	}

	NodeRetreiver::NodeRetreiver(
			const ProcessId& id,
			std::weak_ptr<net::PacketWriters> pWriters,
			const ionet::NodeContainer& nodeContainer)
		: m_id(id)
		, m_pWriters(std::move(pWriters))
		, m_nodeContainer(nodeContainer)
	{}

	void NodeRetreiver::requestNodes(const std::set<ProcessId>& requestedIds) {
		std::lock_guard<std::mutex> guard(m_mutex);

		auto pWriters = m_pWriters.lock();
		if (!pWriters)
			return;

		std::ostringstream out;
		out << "\nDBRB nodes:";
		for (const auto& pair : m_nodes)
			out << "\n" << pair.second.identityKey() << " " << pair.second;
		CATAPULT_LOG(trace) << out.str();

		std::set<ProcessId> ids;
		for (const auto& id : requestedIds) {
			if (m_nodes.find(id) == m_nodes.end())
				ids.emplace(id);
		}
		ids.erase(m_id);
		CATAPULT_LOG(trace) << "[DBRB] requesting " << ids.size() << " nodes";

		if (ids.empty())
			return;

		auto nodes = m_nodeContainer.view().getNodes(ids);

		for (auto& node : nodes) {
			const auto& id = node.identityKey();
			if (node.endpoint().Host.empty()) {
				CATAPULT_LOG(warning) << "[DBRB] empty host returned for " << node << " " << id;
				continue;
			}

			const auto& endpoint = node.endpoint();
			node = ionet::Node(node.identityKey(), ionet::NodeEndpoint{ endpoint.Host, endpoint.DbrbPort }, node.metadata());
			m_nodes[id] = node;
			CATAPULT_LOG(trace) << "[DBRB] Added node " << node << " " << id;
			auto identities = pWriters->identities();
			if ((identities.find(id) != identities.cend())) {
				CATAPULT_LOG(trace) << "[DBRB] Already connected to " << node << " " << id;
				continue;
			}

			CATAPULT_LOG(trace) << "[DBRB] Connecting to " << node << " " << id;
			pWriters->connect(node, [pThisWeak = weak_from_this(), node](const auto& result) {
				CATAPULT_LOG_LEVEL(MapToLogLevel(result.Code)) << "[DBRB] connection attempt to " << node << " " << node.identityKey() << " completed with " << result.Code;
				if (result.Code != net::PeerConnectCode::Accepted && result.Code != net::PeerConnectCode::Already_Connected) {
					auto pThis = pThisWeak.lock();
					if (pThis)
						pThis->removeNode(node.identityKey());
				}
			});
		}
	}

	std::optional<ionet::Node> NodeRetreiver::getNode(const ProcessId& id) const {
		std::lock_guard<std::mutex> guard(m_mutex);
		std::optional<ionet::Node> node;
		auto iter = m_nodes.find(id);
		if (iter != m_nodes.end())
			node = iter->second;

		return node;
	}

	void NodeRetreiver::removeNode(const ProcessId& id) {
		std::lock_guard<std::mutex> guard(m_mutex);
		auto iter = m_nodes.find(id);
		if (iter != m_nodes.end()) {
			CATAPULT_LOG(trace) << "[DBRB] removing node " << iter->second << " " << id;
			m_nodes.erase(iter);
		}
	}
}}