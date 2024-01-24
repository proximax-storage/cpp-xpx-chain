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
}

namespace catapult { namespace dbrb {

	using NodePacketIoPairMap = std::map<ProcessId, ionet::NodePacketIoPair>;

	class MessageSender : public std::enable_shared_from_this<MessageSender> {
	public:
		explicit MessageSender(ionet::Node thisNode, std::weak_ptr<net::PacketWriters> pWriters, const ionet::NodeContainer& nodeContainer);
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
		void requestNodes(const std::set<ProcessId>& requestedIds);
		void addNodes(const std::vector<ionet::Node>& nodes);
		void removeNode(const ProcessId& id);
		void broadcastNodes(const Payload& payload);
		void clearBroadcastData();

	private:
		void broadcastNodes(const std::vector<ionet::Node>& nodes);

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
		const ionet::NodeContainer& m_nodeContainer;
		std::map<ProcessId, ionet::Node> m_nodes;
		std::unordered_set<ProcessId, utils::ArrayHasher<ProcessId>> m_connectionInProgress;
		std::map<ProcessId, std::unordered_set<Hash256, utils::ArrayHasher<Hash256>>> m_broadcastedPackets;
		mutable std::mutex m_nodeMutex;
	};
}}