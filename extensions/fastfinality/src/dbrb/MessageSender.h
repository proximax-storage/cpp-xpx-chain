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

	using NodePacketIoPairMap = std::map<ProcessId, std::shared_ptr<ionet::NodePacketIoPair>>;

	class MessageSender : public std::enable_shared_from_this<MessageSender> {
	public:
		explicit MessageSender(std::weak_ptr<net::PacketWriters> pWriters, NodeRetreiver& nodeRetreiver);

		void send(const std::shared_ptr<MessagePacket>& pPacket, const std::set<ProcessId>& recipients);
		NodePacketIoPairMap& getPacketIos(const std::set<ProcessId>& recipients);

	private:
		std::weak_ptr<net::PacketWriters> m_pWriters;
		NodeRetreiver& m_nodeRetreiver;
		NodePacketIoPairMap m_packetIoPairs;
	};
}}