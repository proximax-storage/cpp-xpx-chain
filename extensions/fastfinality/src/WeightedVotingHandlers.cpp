/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "WeightedVotingHandlers.h"
#include "WeightedVotingFsm.h"
#include "catapult/crypto/KeyUtils.h"
#include "catapult/crypto/Signer.h"
#include "catapult/harvesting_core/UnlockedAccounts.h"
#include "catapult/ionet/PacketPayloadFactory.h"
#include "catapult/model/BlockUtils.h"

namespace catapult { namespace fastfinality {

	void RegisterPushProposedBlockHandler(
			std::weak_ptr<WeightedVotingFsm> pFsmWeak,
			ionet::ServerPacketHandlers& handlers,
			const plugins::PluginManager& pluginManager) {
		handlers.registerHandler(ionet::PacketType::Push_Proposed_Block, [pFsmWeak, &pluginManager](
				const auto& packet, const auto&) {
			TRY_GET_FSM()

			auto& committeeData = pFsmShared->committeeData();
			auto phase = committeeData.committeeStage().Phase;
			if (CommitteePhase::Propose != phase) {
				CATAPULT_LOG(trace) << "rejecting proposal, phase is " << phase;
				return;
			}

			const auto& registry = pluginManager.transactionRegistry();
			auto pBlock = utils::UniqueToShared(ionet::ExtractEntityFromPacket<model::Block>(packet, [&registry](const auto& entity) {
				return IsSizeValid(entity, registry);
			}));
			if (!pBlock) {
				CATAPULT_LOG(warning) << "rejecting invalid packet: " << packet;
				return;
			}

			CATAPULT_LOG(trace) << "received valid " << packet;

			if (committeeData.proposedBlock()) {
				CATAPULT_LOG(trace) << "rejecting proposal, there is one already";
				return;
			}

			committeeData.setProposedBlock(pBlock);

			pFsmShared->proposalWaitTimer().cancel();
		});
	}

	void RegisterPushConfirmedBlockHandler(
			std::weak_ptr<WeightedVotingFsm> pFsmWeak,
			ionet::ServerPacketHandlers& handlers,
			const plugins::PluginManager& pluginManager) {
		handlers.registerHandler(ionet::PacketType::Push_Confirmed_Block, [pFsmWeak, &pluginManager](
				const auto& packet, const auto&) {
			TRY_GET_FSM()

			auto& committeeData = pFsmShared->committeeData();
			auto phase = committeeData.committeeStage().Phase;
			if (CommitteePhase::Commit != phase) {
				CATAPULT_LOG(trace) << "rejecting confirmed block, phase is " << phase;
				return;
			}

			const auto& registry = pluginManager.transactionRegistry();
			auto pBlock = utils::UniqueToShared(ionet::ExtractEntityFromPacket<model::Block>(packet, [&registry](const auto& entity) {
				return IsSizeValid(entity, registry);
			}));
			if (!pBlock) {
				CATAPULT_LOG(warning) << "rejecting invalid packet: " << packet;
				return;
			}

			CATAPULT_LOG(trace) << "received valid " << packet;

			if (committeeData.confirmedBlock()) {
				CATAPULT_LOG(trace) << "rejecting confirmed block, there is one already";
				return;
			}

			committeeData.setConfirmedBlock(pBlock);

			pFsmShared->confirmedBlockWaitTimer().cancel();
		});
	}

	namespace {
		template<typename TPacket>
		void RegisterPushVoteMessageHandler(
				std::weak_ptr<WeightedVotingFsm> pFsmWeak,
				CommitteePhase committeePhase,
				ionet::PacketType packetType,
				ionet::ServerPacketHandlers& handlers,
				const std::string& name) {
			handlers.registerHandler(packetType, [pFsmWeak, committeePhase, name](const auto& packet, auto&) {
				TRY_GET_FSM()

				auto& committeeData = pFsmShared->committeeData();
				auto phase = committeeData.committeeStage().Phase;
				if (phase != committeePhase) {
					CATAPULT_LOG(trace) << "rejecting " << name << ", phase " << phase << ", expected phase " << committeePhase;
					return;
				}

				const auto* pPacket = ionet::CoercePacket<TPacket>(&packet);
				if (!pPacket) {
					CATAPULT_LOG(warning) << "rejecting invalid packet: " << packet;
					return;
				}

				const auto& signer = pPacket->Message.BlockCosignature.Signer;
				if (committeeData.hasVote(signer, pPacket->Message.Type)) {
					CATAPULT_LOG(trace) << "already has vote " << signer << ", phase " << phase;
					return;
				}

				auto pProposedBlock = committeeData.proposedBlock();
				if (!pProposedBlock) {
					CATAPULT_LOG(warning) << "rejecting " << name << ", no proposed block" << ", phase " << phase;
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

				auto pPacketCopy = ionet::CreateSharedPacket<TPacket>();
				std::memcpy(pPacketCopy.get(), pPacket, pPacket->Size);
				committeeData.addVote(std::move(pPacketCopy));
			});
		}
	}

	void RegisterPushPrevoteMessageHandler(
			std::weak_ptr<WeightedVotingFsm> pFsmWeak,
			ionet::ServerPacketHandlers& handlers) {
		RegisterPushVoteMessageHandler<PrevoteMessagePacket>(
			pFsmWeak,
			CommitteePhase::Prevote,
			ionet::PacketType::Push_Prevote_Message,
			handlers,
			"push prevote message");
	}

	void RegisterPushPrecommitMessageHandler(
			std::weak_ptr<WeightedVotingFsm> pFsmWeak,
			ionet::ServerPacketHandlers& handlers) {
		RegisterPushVoteMessageHandler<PrecommitMessagePacket>(
			pFsmWeak,
			CommitteePhase::Precommit,
			ionet::PacketType::Push_Precommit_Message,
			handlers,
			"push precommit message");
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
			auto pResponsePacket = ionet::CreateSharedPacket<RemoteNodeStatePacket>(sizeof(Key) * harvesterKeysCount);

			const auto targetHeight = std::min(lastBlockElementSupplier()->Block.Height, pRequest->Height);
			const auto pBlockElement = blockElementGetter(targetHeight);

			pResponsePacket->Height = pBlockElement->Block.Height;
			pResponsePacket->BlockHash = pBlockElement->EntityHash;
			pResponsePacket->WorkState = pFsmShared->nodeWorkState();
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