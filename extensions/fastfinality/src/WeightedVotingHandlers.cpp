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
#include "catapult/model/BlockUtils.h"

namespace catapult { namespace fastfinality {

	void RegisterPushProposedBlockHandler(
			std::weak_ptr<WeightedVotingFsm> pFsmWeak,
			ionet::ServerPacketHandlers& handlers,
			const plugins::PluginManager& pluginManager) {
		handlers.registerHandler(ionet::PacketType::Push_Proposed_Block, [pFsmWeak, &pluginManager](
				const auto& packet, const auto&) {
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

			committeeData.setProposedBlock(pBlock);
		});
	}

	void RegisterPullProposedBlockHandler(std::weak_ptr<WeightedVotingFsm> pFsmWeak, ionet::ServerPacketHandlers& handlers) {
		handlers.registerHandler(ionet::PacketType::Pull_Proposed_Block, [pFsmWeak](const auto& packet, auto& context) {
			TRY_GET_FSM()

			std::vector<std::shared_ptr<model::Block>> response;
			auto pProposedBlock = pFsmShared->committeeData().proposedBlock();
			if (pProposedBlock)
				response.push_back(pProposedBlock);
			context.response(ionet::PacketPayloadFactory::FromEntities(ionet::PacketType::Pull_Proposed_Block, response));
		});
	}

	void RegisterPushConfirmedBlockHandler(
			std::weak_ptr<WeightedVotingFsm> pFsmWeak,
			ionet::ServerPacketHandlers& handlers,
			const plugins::PluginManager& pluginManager) {
		handlers.registerHandler(ionet::PacketType::Push_Confirmed_Block, [pFsmWeak, &pluginManager](
				const auto& packet, const auto&) {
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
			if (committeeData.confirmedBlock()) {
				CATAPULT_LOG(trace) << "rejecting confirmed block, there is one already";
				return;
			}

			committeeData.setConfirmedBlock(pBlock);
		});
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

	namespace {
		template<typename TPacket>
		void RegisterPushVoteMessageHandler(
				std::weak_ptr<WeightedVotingFsm> pFsmWeak,
				ionet::ServerPacketHandlers& handlers,
				const std::string& name) {
			handlers.registerHandler(TPacket::Packet_Type, [pFsmWeak, name](const auto& packet, auto&) {
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
			});
		}
	}

	void RegisterPushPrevoteMessagesHandler(
			std::weak_ptr<WeightedVotingFsm> pFsmWeak,
			ionet::ServerPacketHandlers& handlers) {
		RegisterPushVoteMessageHandler<PushPrevoteMessagesRequest>(pFsmWeak, handlers, "prevote message");
	}

	void RegisterPushPrecommitMessagesHandler(
			std::weak_ptr<WeightedVotingFsm> pFsmWeak,
			ionet::ServerPacketHandlers& handlers) {
		RegisterPushVoteMessageHandler<PushPrecommitMessagesRequest>(pFsmWeak, handlers, "precommit message");
	}

	namespace {
		template<typename TPacket, CommitteeMessageType MessageType, CommitteePhase Phase>
		void RegisterPullVoteMessagesHandler(
				std::weak_ptr<WeightedVotingFsm> pFsmWeak,
				ionet::ServerPacketHandlers& handlers) {
			handlers.registerHandler(TPacket::Packet_Type, [pFsmWeak](const auto& packet, auto& context) {
				TRY_GET_FSM()

				auto votes = pFsmShared->committeeData().votes(MessageType);
				CATAPULT_LOG(debug) << "returning " << votes.size() << " " << Phase << " votes";
				auto pResponsePacket = ionet::CreateSharedPacket<TPacket>(utils::checked_cast<size_t, uint32_t>(sizeof(CommitteeMessage) * votes.size()));
				pResponsePacket->MessageCount = utils::checked_cast<size_t, uint8_t>(votes.size());

				auto* pMessage = reinterpret_cast<CommitteeMessage*>(pResponsePacket.get() + 1);
				auto index = 0u;
				for (const auto& pair : votes)
					pMessage[index++] = pair.second;

				context.response(ionet::PacketPayload(pResponsePacket));
			});
		}
	}

	void RegisterPullPrevoteMessagesHandler(
			std::weak_ptr<WeightedVotingFsm> pFsmWeak,
			ionet::ServerPacketHandlers& handlers) {
		RegisterPullVoteMessagesHandler<PullPrevoteMessagesRequest, CommitteeMessageType::Prevote, CommitteePhase::Prevote>(pFsmWeak, handlers);
	}

	void RegisterPullPrecommitMessagesHandler(
			std::weak_ptr<WeightedVotingFsm> pFsmWeak,
			ionet::ServerPacketHandlers& handlers) {
		RegisterPullVoteMessagesHandler<PullPrecommitMessagesRequest, CommitteeMessageType::Precommit, CommitteePhase::Precommit>(pFsmWeak, handlers);
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