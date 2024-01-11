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

	namespace {
		std::shared_ptr<model::Block> GetBlockFromPacket(const plugins::PluginManager& pluginManager, const ionet::Packet& packet) {
			const auto& registry = pluginManager.transactionRegistry();
			auto pBlock = utils::UniqueToShared(ionet::ExtractEntityFromPacket<model::Block>(packet, [&registry](const auto& entity) {
				return IsSizeValid(entity, registry);
			}));

			if (!pBlock)
				CATAPULT_LOG(warning) << "rejecting invalid packet: " << packet;

			CATAPULT_LOG(trace) << "received valid " << packet;

			return pBlock;
		}

		bool ValidateProposedBlock(const WeightedVotingFsm& fsm, const plugins::PluginManager& pluginManager, const model::Block& block) {
			const auto& committee = pluginManager.getCommitteeManager(model::Block::Current_Version).committee();
			if (committee.Round < 0) {
				CATAPULT_LOG(warning) << "rejecting proposal, committee is not yet selected";
				return false;
			}

			if (block.Signer != committee.BlockProposer) {
				CATAPULT_LOG(warning) << "rejecting proposal, signer " << block.Signer << " invalid, expected " << committee.BlockProposer;
				return false;
			}

			if (block.round() != committee.Round) {
				CATAPULT_LOG(warning) << "rejecting proposal, round " << block.round() << " invalid, expected " << committee.Round;
				return false;
			}

			if (!model::VerifyBlockHeaderSignature(block)) {
				CATAPULT_LOG(warning) << "rejecting proposal, signature invalid";
				return false;
			}

			return true;
		}
	}

	bool ValidateProposedBlock(const WeightedVotingFsm& fsm, const plugins::PluginManager& pluginManager, const ionet::Packet& packet, std::shared_ptr<model::Block>& pBlock) {
		pBlock = GetBlockFromPacket(pluginManager, packet);
		if (!pBlock)
			return false;

		return ValidateProposedBlock(fsm, pluginManager, *pBlock);
	}

	void PushProposedBlock(
			WeightedVotingFsm& fsm,
			const plugins::PluginManager& pluginManager,
			const ionet::Packet& packet) {
		auto& committeeData = fsm.committeeData();
		if (committeeData.proposedBlock()) {
			CATAPULT_LOG(trace) << "rejecting proposal, there is one already";
			return;
		}

		std::shared_ptr<model::Block> pBlock;
		if (ValidateProposedBlock(fsm, pluginManager, packet, pBlock))
			committeeData.setProposedBlock(pBlock);
	}

	namespace {
		template<typename TPacket>
		bool ValidateVoteMessages(WeightedVotingFsm& fsm, const ionet::Packet& packet, const std::string& name, bool pushVotes) {
			const auto* pPacket = static_cast<const TPacket*>(&packet);
			if (!pPacket) {
				CATAPULT_LOG(warning) << "rejecting invalid packet: " << packet;
				return false;
			}

			CATAPULT_LOG(trace) << "received valid " << packet;

			auto& committeeData = fsm.committeeData();
			const auto* pMessage = reinterpret_cast<const CommitteeMessage*>(pPacket + 1);
			for (uint8_t i = 0u; i < pPacket->MessageCount; ++i, ++pMessage) {
				auto pProposedBlock = committeeData.proposedBlock();
				if (!pProposedBlock) {
					CATAPULT_LOG(warning) << "rejecting " << name << ", no proposed block";
					return false;
				}

				if (pMessage->BlockHash != committeeData.proposedBlockHash()) {
					CATAPULT_LOG(warning) << "rejecting " << name << ", block hash invalid";
					return false;
				}

				const auto& cosignature = pMessage->BlockCosignature;
				if (!crypto::Verify(cosignature.Signer, CommitteeMessageDataBuffer(*pMessage), pMessage->MessageSignature)) {
					CATAPULT_LOG(warning) << "rejecting " << name << ", message signature invalid";
					return false;
				}

				if (!model::VerifyBlockHeaderCosignature(*pProposedBlock, cosignature)) {
					CATAPULT_LOG(warning) << "rejecting " << name << ", block signature invalid";
					return false;
				}

				if (pushVotes && !committeeData.hasVote(pMessage->BlockCosignature.Signer, pMessage->Type)) {
					committeeData.addVote(*pMessage);
					CATAPULT_LOG(debug) << "collected " << committeeData.votes(pMessage->Type).size() << " " << name << "(s)";
				} else if (pushVotes) {
					CATAPULT_LOG(trace) << "already has vote " << pMessage->BlockCosignature.Signer << " (" << name << ")";
				}
			}

			return true;
		}
	}

	bool ValidatePrevoteMessages(WeightedVotingFsm& fsm, const ionet::Packet& packet) {
		return ValidateVoteMessages<PushPrevoteMessagesRequest>(fsm, packet, "prevote message", false);
	}

	void PushPrevoteMessages(WeightedVotingFsm& fsm, const ionet::Packet& packet) {
		ValidateVoteMessages<PushPrevoteMessagesRequest>(fsm, packet, "prevote message", true);
	}

	bool ValidatePrecommitMessages(WeightedVotingFsm& fsm, const ionet::Packet& packet) {
		return ValidateVoteMessages<PushPrecommitMessagesRequest>(fsm, packet, "precommit message", false);
	}

	void PushPrecommitMessages(WeightedVotingFsm& fsm, const ionet::Packet& packet) {
		ValidateVoteMessages<PushPrecommitMessagesRequest>(fsm, packet, "precommit message", true);
	}

	void RegisterPullConfirmedBlockHandler(const std::weak_ptr<WeightedVotingFsm>& pFsmWeak, ionet::ServerPacketHandlers& handlers) {
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
			const std::weak_ptr<WeightedVotingFsm>& pFsmWeak,
			ionet::ServerPacketHandlers& handlers,
			const Key& bootPublicKey,
			const std::function<std::shared_ptr<const model::BlockElement> (const Height&)>& blockElementGetter,
			const model::BlockElementSupplier& lastBlockElementSupplier) {
		handlers.registerHandler(ionet::PacketType::Pull_Remote_Node_State, [pFsmWeak, bootPublicKey, blockElementGetter, lastBlockElementSupplier](
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
			pResponsePacketData[0] = bootPublicKey;
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