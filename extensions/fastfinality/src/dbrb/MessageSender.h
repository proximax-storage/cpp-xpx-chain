/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/dbrb/DbrbDefinitions.h"
#include "catapult/dbrb/DbrbUtils.h"
#include "catapult/ionet/Node.h"
#include <condition_variable>
#include <thread>

namespace catapult {
	namespace ionet {
		class NodeContainer;
		class NodePacketIoPair;
		class Packet;
	}
	namespace net { class PacketWriters; }
	namespace dbrb { class TransactionSender; }
	namespace config { class BlockchainConfigurationHolder; }
}

namespace catapult { namespace dbrb {

	using NodePacketIoPairMap = std::map<ProcessId, ionet::NodePacketIoPair>;

	class MessageSender : public std::enable_shared_from_this<MessageSender> {
	public:
		explicit MessageSender(
			ionet::Node thisNode,
			std::weak_ptr<net::PacketWriters> pWriters,
			const ionet::NodeContainer& nodeContainer,
			bool broadcastThisNode,
			std::shared_ptr<TransactionSender> pTransactionSender,
			const dbrb::DbrbViewFetcher& dbrbViewFetcher);
		~MessageSender();

		// Message sending
		void enqueue(const Payload& payload, const std::set<ProcessId>& recipients);
		void clearQueue();
		ionet::NodePacketIoPair getNodePacketIoPair(const ProcessId& id);
		void pushNodePacketIoPair(const ProcessId& id, const ionet::NodePacketIoPair& nodePacketIoPair);

	private:
		void workerThreadFunc();

	public:
		// Node discovery
		void requestNodes(const std::set<ProcessId>& requestedIds, const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder);
		void addNodes(const std::vector<ionet::Node>& nodes, const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder);
		void removeNode(const ProcessId& id);
		void broadcastNodes(const Payload& payload);
		void broadcastThisNode();
		void clearBroadcastData();
		bool isNodeAdded(const ProcessId& id);
		void addRemoveNodeResponse(const ProcessId& idToRemove, const ProcessId& respondentId, const Timestamp& timestamp, const Signature& signature);

	private:
		void broadcastNodes(const std::vector<ionet::Node>& nodes);

	public:
		void clearNodeRemovalData();

	private:
		std::weak_ptr<net::PacketWriters> m_pWriters;
		NodePacketIoPairMap m_packetIoPairs;

		// Message sending
		using BufferType = std::vector<std::pair<Payload, std::set<ProcessId>>>;
		BufferType m_buffer;
		std::mutex m_messageMutex;
		std::condition_variable m_condVar;
		volatile bool m_running;
		volatile bool m_clearQueue;
		std::thread m_workerThread;

		// Node discovery
		ionet::Node m_thisNode;
		bool m_broadcastThisNode;
		const ionet::NodeContainer& m_nodeContainer;
		std::map<ProcessId, ionet::Node> m_nodes;
		std::unordered_set<ProcessId, utils::ArrayHasher<ProcessId>> m_connectionInProgress;
		std::map<ProcessId, std::unordered_set<Hash256, utils::ArrayHasher<Hash256>>> m_broadcastedPackets;
		mutable std::mutex m_nodeMutex;

		// Node removing
		struct NodeRemovalData {
			catapult::Timestamp Timestamp;
			ViewData View;
			std::map<ProcessId, Signature> Votes;
		};
		std::map<ProcessId, NodeRemovalData> m_nodesToRemove;
		std::shared_ptr<TransactionSender> m_pTransactionSender;
		const dbrb::DbrbViewFetcher& m_dbrbViewFetcher;
		mutable std::mutex m_removeNodeMutex;
	};
}}