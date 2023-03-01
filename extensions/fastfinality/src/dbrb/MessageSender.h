/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "NodeRetreiver.h"

namespace catapult {
	namespace ionet { class NodePacketIoPair; }
	namespace net { class PacketWriters; }
	namespace dbrb { class MessagePacket; }
}

namespace catapult { namespace dbrb {

	class MessageSender
			: public AsyncMessageQueue<std::pair<std::shared_ptr<MessagePacket>, std::set<ProcessId>>>
			, public std::enable_shared_from_this<MessageSender> {
	public:
		explicit MessageSender(std::shared_ptr<net::PacketWriters> pWriters, NodeRetreiver& nodeRetreiver);
		~MessageSender() override;

	private:
		void processBuffer(BufferType& buffer) override;

	private:
		std::shared_ptr<net::PacketWriters> m_pWriters;
		NodeRetreiver& m_nodeRetreiver;
		std::map<ProcessId, std::shared_ptr<ionet::NodePacketIoPair>> m_packetIoPairs;
	};
}}