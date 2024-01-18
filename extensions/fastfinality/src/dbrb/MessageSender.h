/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "NodeRetreiver.h"
#include <condition_variable>
#include <thread>

namespace catapult {
	namespace ionet { class NodePacketIoPair; }
	namespace net { class PacketWriters; }
	namespace dbrb { class MessagePacket; }
}

namespace catapult { namespace dbrb {

	using NodePacketIoPairMap = std::map<ProcessId, std::shared_ptr<ionet::NodePacketIoPair>>;

	class MessageSender : public std::enable_shared_from_this<MessageSender> {
	public:
		explicit MessageSender(std::weak_ptr<net::PacketWriters> pWriters, NodeRetreiver& nodeRetreiver);
		~MessageSender();

		NodePacketIoPairMap& getPacketIos(const std::set<ProcessId>& recipients);
		void enqueue(const std::shared_ptr<MessagePacket>& pPacket, const std::set<ProcessId>& recipients);
		void clearQueue();

	private:
		void workerThreadFunc();

	private:
		std::weak_ptr<net::PacketWriters> m_pWriters;
		NodeRetreiver& m_nodeRetreiver;
		NodePacketIoPairMap m_packetIoPairs;

		using BufferType = std::vector<std::pair<std::shared_ptr<MessagePacket>, std::set<ProcessId>>>;
		BufferType m_buffer;
		std::mutex m_mutex;
		std::condition_variable m_condVar;
		bool m_running;
		std::thread m_workerThread;
	};
}}