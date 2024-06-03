/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/ionet/PacketPayload.h"
#include "catapult/model/Cosignature.h"

namespace catapult { namespace fastfinality {

#pragma pack(push, 1)

	enum class NodeWorkState : uint8_t {
		None,
		Synchronizing,
		Running,
	};

	struct RemoteNodeState {
		catapult::Height Height;
		Hash256 BlockHash;
		fastfinality::NodeWorkState NodeWorkState;
		Key NodeKey;
		std::vector<Key> HarvesterKeys;
	};

	/// A retriever that returns remote node states from all available peers.
	using RemoteNodeStateRetriever = std::function<std::vector<RemoteNodeState> ()>;

	struct RemoteNodeStatePacket : public ionet::Packet {
		static constexpr ionet::PacketType Packet_Type = ionet::PacketType::Pull_Remote_Node_State;

		catapult::Height Height;
		Hash256 BlockHash;
		fastfinality::NodeWorkState NodeWorkState = NodeWorkState::None;
		uint8_t HarvesterKeysCount;
	};

	struct RemoteNodeStateTraits {
	public:
		using ResultType = RemoteNodeState;
		static constexpr auto Packet_Type = ionet::PacketType::Pull_Remote_Node_State;
		static constexpr auto Friendly_Name = "remote node state";

		static auto CreateRequestPacketPayload(Height height) {
			auto pPacket = ionet::CreateSharedPacket<RemoteNodeStatePacket>();
			pPacket->Height = std::move(height);
			return ionet::PacketPayload(pPacket);
		}

	public:
		bool tryParseResult(const ionet::Packet& packet, ResultType& result) const {
			const auto* pResponse = static_cast<const RemoteNodeStatePacket*>(&packet);
			const auto* pResponseData = reinterpret_cast<const Key*>(pResponse + 1);

			result.Height = pResponse->Height;
			result.BlockHash = pResponse->BlockHash;
			result.NodeWorkState = pResponse->NodeWorkState;
			for (auto i = 0; i < pResponse->HarvesterKeysCount; ++i) {
				result.HarvesterKeys.push_back(pResponseData[i]);
			}

			return true;
		}
	};

#pragma pack(pop)
}}