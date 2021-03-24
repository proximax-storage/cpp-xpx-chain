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
#include <catapult/ionet/PacketPayloadFactory.h>

#include <utility>
#include "catapult/model/Cosignature.h"

namespace catapult { namespace fastfinality {

#pragma pack(push, 1)

	enum class CommitteeMessageType : uint8_t {
		Prevote,
		Precommit,
	};

	enum class NodeWorkState : uint8_t {
		None,
		Synchronizing,
		Running,
	};

	struct RemoteNodeState {
		Height ChainHeight;
		Hash256 BlockHash;
		NodeWorkState NodeWorkState;
		Key PublicKey;
	};

	// TODO: Same PacketType for both request and response?
	struct RemoteNodeStateRequest : public ionet::Packet {
		static constexpr ionet::PacketType Packet_Type = ionet::PacketType::Pull_Remote_Node_State;
		Height TargetHeight;
	};

	struct RemoteNodeStateResponse : public ionet::Packet {
		static constexpr ionet::PacketType Packet_Type = ionet::PacketType::Pull_Remote_Node_State;

		Height ChainHeight;
		Hash256 BlockHash;
		NodeWorkState NodeWorkState = NodeWorkState::None;
	};

	struct RemoteNodeStateTraits {
	public:
		using ResultType = RemoteNodeState;
		static constexpr auto Packet_Type = ionet::PacketType::Pull_Remote_Node_State;
		static constexpr auto Friendly_Name = "remote node state";

		static auto CreateRequestPacketPayload(Height height) {
			auto pPacket = ionet::CreateSharedPacket<RemoteNodeStateRequest>();
			pPacket->TargetHeight = std::move(height);
			return ionet::PacketPayload(pPacket);
		}

		/*static auto CreateRequestPacketPayload() {
			return ionet::PacketPayload(Packet_Type);
		}*/

	public:
		bool tryParseResult(const ionet::Packet& packet, ResultType& result) const {
			const auto* pResponse = ionet::CoercePacket<RemoteNodeStateResponse>(&packet);
			if (!pResponse)
				return false;

			result.ChainHeight = pResponse->ChainHeight;
			result.BlockHash = pResponse->BlockHash;
			result.NodeWorkState = pResponse->NodeWorkState;

			return true;
		}
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