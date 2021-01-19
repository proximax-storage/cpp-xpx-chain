/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "CommitteePhase.h"
#include "catapult/api/RemoteApiUtils.h"
#include "catapult/ionet/Packet.h"
#include "catapult/ionet/PacketEntityUtils.h"
#include "catapult/ionet/PacketPayload.h"
#include "catapult/model/Cosignature.h"

namespace catapult { namespace fastfinality {

#pragma pack(push, 1)

	struct CommitteeStageResponse : public ionet::Packet {
		static constexpr ionet::PacketType Packet_Type = ionet::PacketType::Pull_Committee_Stage;

		int16_t Round = -1;
		CommitteePhase Phase = CommitteePhase::None;
		Timestamp RoundStart;
		uint64_t PhaseTimeMillis = 0u;
	};

	struct CommitteeStageTraits {
	public:
		using ResultType = CommitteeStage;
		static constexpr auto Packet_Type = ionet::PacketType::Pull_Committee_Stage;
		static constexpr auto Friendly_Name = "committee stage";

		static auto CreateRequestPacketPayload() {
			return ionet::PacketPayload(Packet_Type);
		}

	public:
		bool tryParseResult(const ionet::Packet& packet, ResultType& result) const {
			const auto* pResponse = ionet::CoercePacket<CommitteeStageResponse>(&packet);
			if (!pResponse)
				return false;

			result.Round = pResponse->Round;
			result.Phase = pResponse->Phase;
			result.RoundStart = utils::ToTimePoint(pResponse->RoundStart);
			result.PhaseTimeMillis = pResponse->PhaseTimeMillis;
			return true;
		}
	};

	enum class CommitteeMessageType : uint8_t {
		Prevote,
		Precommit,
	};

	struct CommitteeMessage {
		CommitteeMessageType Type;
		Hash256 BlockHash;
		model::Cosignature BlockCosignature;
	};

	template<ionet::PacketType PacketType, CommitteeMessageType MessageType>
	struct CommitteeMessagePacket : public ionet::Packet {
		static constexpr ionet::PacketType Packet_Type = PacketType;
		static constexpr CommitteeMessageType Message_Type = MessageType;

		CommitteeMessage Message;
		Signature MessageSignature;
	};

	template<ionet::PacketType PacketType, CommitteeMessageType MessageType>
	RawBuffer CommitteeMessageDataBuffer(const CommitteeMessagePacket<PacketType, MessageType>& packet) {
		return {
			reinterpret_cast<const uint8_t*>(&packet) + sizeof(ionet::Packet),
			sizeof(CommitteeMessage)
		};
	}

	using PrevoteMessagePacket = CommitteeMessagePacket<ionet::PacketType::Push_Prevote_Message, CommitteeMessageType::Prevote>;
	using PrecommitMessagePacket = CommitteeMessagePacket<ionet::PacketType::Push_Precommit_Message, CommitteeMessageType::Precommit>;

#pragma pack(pop)
}}