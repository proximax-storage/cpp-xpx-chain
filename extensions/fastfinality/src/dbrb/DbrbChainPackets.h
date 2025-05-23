/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/ionet/Packet.h"
#include "catapult/dbrb/DbrbDefinitions.h"

namespace catapult { namespace dbrb {

#pragma pack(push, 1)

	struct DbrbPushNodesPacket : public ionet::Packet {
		static constexpr ionet::PacketType Packet_Type = ionet::PacketType::Dbrb_Push_Nodes;

		uint16_t NodeCount;
	};

	struct DbrbPullNodesPacket : public ionet::Packet {
		static constexpr ionet::PacketType Packet_Type = ionet::PacketType::Dbrb_Pull_Nodes;

		uint16_t NodeCount;
	};

#pragma pack(pop)
}}