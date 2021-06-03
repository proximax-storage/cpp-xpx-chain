/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/ionet/Packet.h"

namespace catapult { namespace storage {

#pragma pack(push, 1)
		template<ionet::PacketType packetType>
		struct FileDownloadPacket : public ionet::Packet {
			static constexpr ionet::PacketType Packet_Type = packetType;

			Key DriveKey;
			uint16_t FileCount;
		};

#pragma pack(pop)
}}