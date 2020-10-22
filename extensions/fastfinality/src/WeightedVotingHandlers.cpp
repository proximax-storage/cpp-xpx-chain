/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "WeightedVotingHandlers.h"
#include "WeightedVotingFsm.h"
#include "catapult/crypto/Signer.h"
#include "catapult/ionet/PacketPayloadFactory.h"
#include "catapult/model/BlockUtils.h"

namespace catapult { namespace fastfinality {

	void RegisterPullCommitteeStageHandler(
			const std::shared_ptr<WeightedVotingFsm>& pFsm,
			ionet::ServerPacketHandlers& handlers) {
		handlers.registerHandler(ionet::PacketType::Pull_Committee_Stage, [pFsm](
				const auto& packet,
				auto& context) {
			using ResponseType = CommitteeStageResponse;
			if (!ionet::IsPacketValid(packet, ResponseType::Packet_Type)) {
				CATAPULT_LOG(warning) << "rejecting invalid packet: " << packet;
				return;
			}

			CATAPULT_LOG(trace) << "received valid " << packet;
			auto pResponsePacket = ionet::CreateSharedPacket<ResponseType>();
			auto stage = pFsm->committeeData().committeeStage();
			pResponsePacket->Round = stage.Round;
			pResponsePacket->Phase = stage.Phase;
			pResponsePacket->RoundStart = utils::FromTimePoint(stage.RoundStart);
			pResponsePacket->PhaseTimeMillis = stage.PhaseTimeMillis;

			context.response(ionet::PacketPayload(pResponsePacket));
		});
	}

	void RegisterPushProposedBlockHandler(
			const std::shared_ptr<WeightedVotingFsm>& pFsm,
			ionet::ServerPacketHandlers& handlers,
			const plugins::PluginManager& pluginManager) {
		handlers.registerHandler(ionet::PacketType::Push_Proposed_Block, [pFsm, &pluginManager](
				const auto& packet,
				const auto&) {
			auto& committeeData = pFsm->committeeData();
			auto phase = committeeData.committeeStage().Phase;
			if (CommitteePhase::Propose != phase) {
				CATAPULT_LOG(warning) << "rejecting proposal, phase is " << phase;
				return;
			}

			const auto& registry = pluginManager.transactionRegistry();
			auto range = ionet::ExtractEntitiesFromPacket<model::Block>(packet, [&registry](const auto& entity) {
				return IsSizeValid(entity, registry);
			});
			if (range.empty()) {
				CATAPULT_LOG(warning) << "rejecting empty range: " << packet;
				return;
			} else if (range.size() > 1u) {
				CATAPULT_LOG(warning) << "rejecting multiple range: " << packet;
				return;
			}

			CATAPULT_LOG(trace) << "received valid " << packet;
			auto blocks = model::BlockRange::ExtractEntitiesFromRange(std::move(range));
			auto pBlock = blocks.front();

			const auto& committee = pluginManager.getCommitteeManager().committee();
			if (pBlock->Signer != committee.BlockProposer) {
				CATAPULT_LOG(warning) << "rejecting proposal (signer invalid " << pBlock->Signer
					<< ", expected " << committee.BlockProposer << ")";
				return;
			}

			if (!model::VerifyBlockHeaderSignature(*pBlock)) {
				CATAPULT_LOG(warning) << "rejecting proposal (signature invalid) from " << committee.BlockProposer;
				return;
			}

			auto pProposedBlock = committeeData.proposedBlock();
			if (!!pProposedBlock) {
				if (*pProposedBlock != *pBlock) {
					CATAPULT_LOG(warning) << "rejecting multiple proposals from " << committee.BlockProposer;
					committeeData.setProposalMultiple(true);
				}
				return;
			}

			committeeData.setProposedBlock(pBlock);
		});
	}

	void RegisterPullProposedBlockHandler(
			const std::shared_ptr<WeightedVotingFsm>& pFsm,
			ionet::ServerPacketHandlers& handlers) {
		handlers.registerHandler(ionet::PacketType::Pull_Proposed_Block, [pFsm](
				const auto& packet,
				auto& context) {
			if (!ionet::IsPacketValid(packet, ionet::PacketType::Pull_Proposed_Block)) {
				CATAPULT_LOG(warning) << "rejecting invalid packet: " << packet;
				return;
			}

			auto pProposedBlock = pFsm->committeeData().proposedBlock();
			if (!pProposedBlock) {
				CATAPULT_LOG(trace) << "pull proposed block failed, no proposed block";
				return;
			}

			CATAPULT_LOG(trace) << "received valid " << packet;
			context.response(ionet::PacketPayloadFactory::FromEntity(ionet::PacketType::Pull_Proposed_Block, std::move(pProposedBlock)));
		});
	}

	namespace {
		template<typename TPacket>
		void RegisterPushVoteMessageHandler(
			const std::shared_ptr<WeightedVotingFsm>& pFsm,
			consumer<const Key&, const Signature&> addVote,
			CommitteePhase committeePhase,
			ionet::PacketType packetType,
			ionet::ServerPacketHandlers& handlers,
			const std::string& name) {
			handlers.registerHandler(packetType, [pFsm, addVote, committeePhase, &name](
					const auto& packet,
					auto&) {
				auto& committeeData = pFsm->committeeData();
				auto phase = committeeData.committeeStage().Phase;
				if (phase != committeePhase) {
					CATAPULT_LOG(trace) << "rejecting " << name << ", phase " << phase << ", expected phase" << committeePhase;
					return;
				}

				auto pProposedBlock = committeeData.proposedBlock();
				if (!pProposedBlock) {
					CATAPULT_LOG(trace) << "rejecting " << name << ", no proposed block";
					return;
				}

				const auto* pPacket = ionet::CoercePacket<TPacket>(&packet);
				if (!pPacket) {
					CATAPULT_LOG(warning) << "rejecting invalid packet: " << packet;
					return;
				}

				CATAPULT_LOG(trace) << "received valid " << packet;
				if (pPacket->Message.BlockHash != committeeData.proposedBlockHash()) {
					CATAPULT_LOG(warning) << "rejecting " << name << ", block hash invalid";
					return;
				}

				const auto& cosignature = pPacket->Message.BlockCosignature;
				if (!crypto::Verify(cosignature.Signer, CommitteeMessageDataBuffer(*pPacket), pPacket->MessageSignature)) {
					CATAPULT_LOG(warning) << "rejecting " << name << ", message signature invalid";
					return;
				}

				if (!model::VerifyBlockHeaderCosignature(*pProposedBlock, cosignature)) {
					CATAPULT_LOG(warning) << "rejecting " << name << ", block signature invalid";
					return;
				}

				addVote(cosignature.Signer, cosignature.Signature);
			});
		}
	}

	void RegisterPushPrevoteMessageHandler(
			const std::shared_ptr<WeightedVotingFsm>& pFsm,
			ionet::ServerPacketHandlers& handlers) {
		RegisterPushVoteMessageHandler<PrevoteMessagePacket>(
			pFsm,
			[pFsm](const Key& signer, const Signature& signature) { pFsm->committeeData().addPrevote(signer, signature); },
			CommitteePhase::Prevote,
			ionet::PacketType::Push_Prevote_Message,
			handlers,
			"push prevote message");
	}

	void RegisterPushPrecommitMessageHandler(
			const std::shared_ptr<WeightedVotingFsm>& pFsm,
			ionet::ServerPacketHandlers& handlers) {
		RegisterPushVoteMessageHandler<PrecommitMessagePacket>(
			pFsm,
			[pFsm](const Key& signer, const Signature& signature) { pFsm->committeeData().addPrecommit(signer, signature); },
			CommitteePhase::Precommit,
			ionet::PacketType::Push_Precommit_Message,
			handlers,
			"push precommit message");
	}
}}