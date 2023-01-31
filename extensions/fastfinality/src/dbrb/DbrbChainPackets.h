/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "SignedNode.h"
#include "catapult/dbrb/DbrbUtils.h"
#include "catapult/crypto/Signer.h"
#include "catapult/dbrb/DbrbDefinitions.h"
#include "catapult/ionet/NetworkNode.h"
#include "catapult/ionet/PacketPayload.h"

namespace catapult { namespace dbrb {

#pragma pack(push, 1)

	struct DbrbPushNodesPacket : public ionet::Packet {
		static constexpr ionet::PacketType Packet_Type = ionet::PacketType::Dbrb_Push_Nodes;

		uint16_t NodeCount;
	};

	struct DbrbPullNodesPacket : public ionet::Packet {
		static constexpr ionet::PacketType Packet_Type = ionet::PacketType::Dbrb_Pull_Nodes;

		uint16_t ProcessIdCount;
		uint16_t NodeCount;
	};

	template<typename TPacket>
	std::vector<SignedNode> ReadNodesFromPacket(const ionet::Packet& packet, model::NetworkIdentifier networkIdentifier) {
		std::vector<SignedNode> nodes;
		const auto* pPacket = static_cast<const TPacket*>(&packet);
		const auto* pBuffer = reinterpret_cast<const uint8_t*>(pPacket + 1);

		for (auto i = 0; i < pPacket->NodeCount; ++i) {
			const auto* pNetworkNode = reinterpret_cast<const ionet::NetworkNode*>(pBuffer);
			if (pNetworkNode->NetworkIdentifier != networkIdentifier)
				continue;
			pBuffer += pNetworkNode->Size;

			Signature signature;
			std::memcpy(signature.data(), pBuffer, Signature_Size);
			pBuffer += Signature_Size;

			auto hash = CalculateHash({ { reinterpret_cast<const uint8_t*>(pNetworkNode), pNetworkNode->Size } });
			if (!crypto::Verify(pNetworkNode->IdentityKey, hash, signature)) {
				CATAPULT_LOG(warning) << "invalid signature of node with public key" << pNetworkNode->IdentityKey;
				continue;
			}

			auto node = ionet::UnpackNode(*pNetworkNode);
			nodes.emplace_back(SignedNode{ node, signature });
		}

		return nodes;
	}

	struct DbrbPullNodesTraits {
	public:
		using ResultType = std::vector<SignedNode>;
		static constexpr auto Packet_Type = ionet::PacketType::Dbrb_Pull_Nodes;
		static constexpr auto Friendly_Name = "pull dbrb nodes";

		DbrbPullNodesTraits(model::NetworkIdentifier networkIdentifier)
			: m_networkIdentifier(networkIdentifier)
		{}

		static auto CreateRequestPacketPayload(std::set<ProcessId> ids) {
			uint16_t idCount = utils::checked_cast<size_t, uint16_t>(ids.size());
			uint32_t payloadSize = utils::checked_cast<size_t, uint32_t>(ProcessId_Size * idCount);
			auto pPacket = ionet::CreateSharedPacket<DbrbPullNodesPacket>(payloadSize);
			pPacket->NodeCount = 0;
			pPacket->ProcessIdCount = idCount;
			auto* pBuffer = reinterpret_cast<uint8_t*>(pPacket.get() + 1);
			for (const auto& id : ids) {
				std::memcpy(pBuffer, id.data(), ProcessId_Size);
				pBuffer += ProcessId_Size;
			}
			return ionet::PacketPayload(pPacket);
		}

	public:
		bool tryParseResult(const ionet::Packet& packet, ResultType& result) const {
			result = ReadNodesFromPacket<DbrbPullNodesPacket>(packet, m_networkIdentifier);

			return true;
		}

	private:
		model::NetworkIdentifier m_networkIdentifier;
	};

#pragma pack(pop)
}}