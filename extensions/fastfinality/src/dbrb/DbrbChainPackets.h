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

	struct DbrbRemoveNodeRequestPacket : public ionet::Packet {
		static constexpr ionet::PacketType Packet_Type = ionet::PacketType::Dbrb_Remove_Node_Request;

		catapult::Timestamp Timestamp;
		dbrb::ProcessId ProcessId;
	};

	struct DbrbRemoveNodeResponsePacket : public ionet::Packet {
		static constexpr ionet::PacketType Packet_Type = ionet::PacketType::Dbrb_Remove_Node_Response;

		catapult::Timestamp Timestamp;
		dbrb::ProcessId ProcessId;
		catapult::Signature Signature;
	};

#pragma pack(pop)
}}