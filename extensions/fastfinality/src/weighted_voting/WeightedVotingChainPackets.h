/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "CommitteePhase.h"
#include "fastfinality/src/FastFinalityChainPackets.h"

namespace catapult { namespace fastfinality {

#pragma pack(push, 1)

	enum class CommitteeMessageType : uint8_t {
		Prevote,
		Precommit,
	};

	struct CommitteeMessage {
		CommitteeMessageType Type;
		Hash256 BlockHash;
		model::Cosignature BlockCosignature;
		Signature MessageSignature;
	};

	template<ionet::PacketType PacketType>
	struct CommitteeMessagesPacket : public ionet::Packet {
		static constexpr ionet::PacketType Packet_Type = PacketType;

		uint8_t MessageCount;
	};

	using PushPrevoteMessagesRequest = CommitteeMessagesPacket<ionet::PacketType::Push_Prevote_Messages>;
	using PushPrecommitMessagesRequest = CommitteeMessagesPacket<ionet::PacketType::Push_Precommit_Messages>;

#pragma pack(pop)
}}