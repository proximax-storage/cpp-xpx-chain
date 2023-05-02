/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "WeightedVotingHandlers.h"
#include "WeightedVotingFsm.h"
#include "utils/WeightedVotingUtils.h"
#include "catapult/crypto/KeyUtils.h"
#include "catapult/crypto/Signer.h"
#include "catapult/harvesting_core/UnlockedAccounts.h"
#include "catapult/ionet/PacketEntityUtils.h"
#include "catapult/ionet/PacketPayloadFactory.h"
#include "catapult/model/BlockUtils.h"

namespace catapult { namespace fastfinality {

	void PushProposedBlock(
			std::weak_ptr<WeightedVotingFsm> pFsmWeak,
			const plugins::PluginManager& pluginManager,
			const ionet::Packet& packet) {
		TRY_GET_FSM()

		const auto& registry = pluginManager.transactionRegistry();
		auto pBlock = utils::UniqueToShared(ionet::ExtractEntityFromPacket<model::Block>(packet, [&registry](const auto& entity) {
			return IsSizeValid(entity, registry);
		}));
		if (!pBlock) {
			CATAPULT_LOG(warning) << "rejecting invalid packet: " << packet;
			return;
		}

		CATAPULT_LOG(trace) << "received valid " << packet;

		auto& committeeData = pFsmShared->committeeData();
		if (committeeData.proposedBlock()) {
			CATAPULT_LOG(trace) << "rejecting proposal, there is one already";
			return;
		}

		const auto& committee = pluginManager.getCommitteeManager().committee();
		if (pBlock->Signer != committee.BlockProposer) {
			CATAPULT_LOG(warning) << "rejecting proposal, signer " << pBlock->Signer << " invalid, expected " << committee.BlockProposer;
			return;
		}

		if (pBlock->round() != committee.Round) {
			CATAPULT_LOG(warning) << "rejecting proposal, round " << pBlock->round() << " invalid, expected " << committee.Round;
			return;
		}

		if (!model::VerifyBlockHeaderSignature(*pBlock)) {
			CATAPULT_LOG(warning) << "rejecting proposal, signature invalid";
			return;
		}

		committeeData.setProposedBlock(pBlock);
	}

	namespace {
		template<typename TPacket>
		void PushVoteMessages(std::weak_ptr<WeightedVotingFsm> pFsmWeak, const ionet::Packet& packet, const std::string& name) {
			TRY_GET_FSM()

			const auto* pPacket = static_cast<const TPacket*>(&packet);
			if (!pPacket) {
				CATAPULT_LOG(warning) << "rejecting invalid packet: " << packet;
				return;
			}

			auto& committeeData = pFsmShared->committeeData();
			const auto* pMessage = reinterpret_cast<const CommitteeMessage*>(pPacket + 1);
			for (uint8_t i = 0u; i < pPacket->MessageCount; ++i, ++pMessage) {
				const auto& signer = pMessage->BlockCosignature.Signer;
				if (committeeData.hasVote(signer, pMessage->Type)) {
					CATAPULT_LOG(trace) << "already has vote " << signer << " (" << name << ")";
					continue;
				}

				auto pProposedBlock = committeeData.proposedBlock();
				if (!pProposedBlock) {
					CATAPULT_LOG(warning) << "rejecting " << name << ", no proposed block";
					continue;
				}

				CATAPULT_LOG(trace) << "received valid " << packet;
				if (pMessage->BlockHash != committeeData.proposedBlockHash()) {
					CATAPULT_LOG(warning) << "rejecting " << name << ", block hash invalid";
					continue;
				}

				const auto& cosignature = pMessage->BlockCosignature;
				if (!crypto::Verify(cosignature.Signer, CommitteeMessageDataBuffer(*pMessage), pMessage->MessageSignature)) {
					CATAPULT_LOG(warning) << "rejecting " << name << ", message signature invalid";
					continue;
				}

				if (!model::VerifyBlockHeaderCosignature(*pProposedBlock, cosignature)) {
					CATAPULT_LOG(warning) << "rejecting " << name << ", block signature invalid";
					continue;
				}

				committeeData.addVote(*pMessage);

				CATAPULT_LOG(debug) << "collected " << committeeData.votes(pMessage->Type).size() << " " << name << "(s)";
			}
		}
	}

	void PushPrevoteMessages(std::weak_ptr<WeightedVotingFsm> pFsmWeak, const ionet::Packet& packet) {
		PushVoteMessages<PushPrevoteMessagesRequest>(pFsmWeak, packet, "prevote message");
	}

	void PushPrecommitMessages(std::weak_ptr<WeightedVotingFsm> pFsmWeak, const ionet::Packet& packet) {
		PushVoteMessages<PushPrecommitMessagesRequest>(pFsmWeak, packet, "precommit message");
	}

	void RegisterPullConfirmedBlockHandler(std::weak_ptr<WeightedVotingFsm> pFsmWeak, ionet::ServerPacketHandlers& handlers) {
		handlers.registerHandler(ionet::PacketType::Pull_Confirmed_Block, [pFsmWeak](
				const auto& packet, auto& context) {
			TRY_GET_FSM()

			std::vector<std::shared_ptr<model::Block>> response;
			auto pConfirmedBlock = pFsmShared->committeeData().confirmedBlock();
			if (pConfirmedBlock)
				response.push_back(pConfirmedBlock);
			context.response(ionet::PacketPayloadFactory::FromEntities(ionet::PacketType::Pull_Confirmed_Block, response));
		});
	}

	void RegisterPullRemoteNodeStateHandler(
			std::weak_ptr<WeightedVotingFsm> pFsmWeak,
			ionet::ServerPacketHandlers& handlers,
			const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder,
			const std::function<std::shared_ptr<const model::BlockElement> (const Height&)>& blockElementGetter,
			const model::BlockElementSupplier& lastBlockElementSupplier) {
		handlers.registerHandler(ionet::PacketType::Pull_Remote_Node_State, [pFsmWeak, pConfigHolder, blockElementGetter, lastBlockElementSupplier](
				const auto& packet,
				auto& context) {
			const auto pRequest = ionet::CoercePacket<RemoteNodeStatePacket>(&packet);
			if (!pRequest) {
				CATAPULT_LOG(warning) << "rejecting empty request: " << packet;
				return;
			}

			CATAPULT_LOG(trace) << "received valid " << packet;

			TRY_GET_FSM()

			const auto& view = pFsmShared->committeeData().unlockedAccounts()->view();
			const uint8_t harvesterKeysCount = 1 + view.size();	// Extra one for a BootKey
			auto pResponsePacket = ionet::CreateSharedPacket<RemoteNodeStatePacket>(Key_Size * harvesterKeysCount);

			const auto targetHeight = std::min(lastBlockElementSupplier()->Block.Height, pRequest->Height);
			const auto pBlockElement = blockElementGetter(targetHeight);

			pResponsePacket->Height = pBlockElement->Block.Height;
			pResponsePacket->BlockHash = pBlockElement->EntityHash;
			pResponsePacket->NodeWorkState = pFsmShared->nodeWorkState();
			pResponsePacket->HarvesterKeysCount = harvesterKeysCount;

			auto* pResponsePacketData = reinterpret_cast<Key*>(pResponsePacket.get() + 1);
			pResponsePacketData[0] = crypto::ParseKey(pConfigHolder->Config().User.BootKey);
			{
				auto iter = view.begin();
				auto index = 1;
				while (iter != view.end()) {
					pResponsePacketData[index] = iter->publicKey();
					++iter;
					++index;
				}
			}

			context.response(ionet::PacketPayload(pResponsePacket));
		});
	}
}}