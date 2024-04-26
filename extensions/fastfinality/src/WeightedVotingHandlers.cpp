/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "WeightedVotingHandlers.h"
#include "WeightedVotingFsm.h"
#include "utils/WeightedVotingUtils.h"
#include "catapult/chain/BlockDifficultyScorer.h"
#include "catapult/chain/ChainUtils.h"
#include "catapult/crypto/KeyUtils.h"
#include "catapult/crypto/Signer.h"
#include "catapult/extensions/ExecutionConfigurationFactory.h"
#include "catapult/extensions/PluginUtils.h"
#include "catapult/harvesting_core/UnlockedAccounts.h"
#include "catapult/ionet/PacketEntityUtils.h"
#include "catapult/ionet/PacketPayloadFactory.h"
#include "catapult/model/BlockUtils.h"
#include "catapult/validators/AggregateEntityValidator.h"

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

		model::MatchingEntityPredicate ToRequiresValidationPredicate(const chain::KnownHashPredicate& knownHashPredicate) {
			return [knownHashPredicate](auto entityType, auto timestamp, const auto& hash) {
				auto isTransaction = model::BasicEntityType::Transaction == entityType;
				return !isTransaction || !knownHashPredicate(timestamp, hash);
			};
		}

		consumers::BlockChainSyncHandlers CreateBlockChainSyncHandlers(extensions::ServiceState& state) {
			const auto& pluginManager = state.pluginManager();
			consumers::BlockChainSyncHandlers syncHandlers;
			syncHandlers.DifficultyChecker = [&pluginManager](
					const auto& blocks,
					const cache::CatapultCache& cache,
					const model::NetworkConfigurations& remoteConfigs) {
				if (!blocks.size())
					return true;
				auto result = chain::CheckDifficulties(cache.sub<cache::BlockDifficultyCache>(), blocks, pluginManager.configHolder(), remoteConfigs);
				return blocks.size() == result;
			};

			syncHandlers.Processor = consumers::CreateBlockChainProcessor(
				[](const cache::ReadOnlyCatapultCache&) {
					return [](const model::Block&, const model::Block&, const GenerationHash&) {
						return true;
					};
				},
				chain::CreateBatchEntityProcessor(extensions::CreateExecutionConfiguration(pluginManager)),
				state);

			return syncHandlers;
		}

		auto CreateBlockConsumers(
				extensions::ServiceState& state,
				const model::BlockElementSupplier& lastBlockElementSupplier,
				const std::shared_ptr<thread::IoThreadPool>& pValidatorPool) {
			std::vector<disruptor::BlockConsumer> blockConsumers;
			blockConsumers.push_back(consumers::CreateBlockHashCalculatorConsumer(
				state.config().Immutable.GenerationHash,
				state.pluginManager().transactionRegistry()));
			blockConsumers.emplace_back(consumers::CreateBlockChainCheckConsumer(
				state.config().Node.MaxBlocksPerSyncAttempt,
				state.pluginManager().configHolder(),
				state.timeSupplier()));
			blockConsumers.emplace_back(consumers::CreateBlockStatelessValidationConsumer(
				extensions::CreateStatelessValidator(state.pluginManager()),
				validators::CreateParallelValidationPolicy(pValidatorPool),
				ToRequiresValidationPredicate(state.hooks().knownHashPredicate(state.utCache()))));
			blockConsumers.push_back(consumers::CreateBlockValidatorConsumer(
				state.cache(),
				state.state(),
				CreateBlockChainSyncHandlers(state),
				lastBlockElementSupplier));

			return blockConsumers;
		}

		bool ValidateBlock(
				const model::Block& block,
				bool isProposedBlock,
				extensions::ServiceState& state,
				const model::BlockElementSupplier& lastBlockElementSupplier,
				const std::shared_ptr<thread::IoThreadPool>& pValidatorPool) {
			auto blockConsumers = CreateBlockConsumers(state, lastBlockElementSupplier, pValidatorPool);
			auto name = (isProposedBlock ? "proposed" : "confirmed");
			try {
				std::vector<model::BlockElement> blocks{ model::BlockElement(block) };
				for (const auto& blockConsumer : blockConsumers) {
					auto result = blockConsumer(blocks);
					if (disruptor::CompletionStatus::Aborted == result.CompletionStatus) {
						auto validationResult = static_cast<validators::ValidationResult>(result.CompletionCode);
						CATAPULT_LOG_LEVEL(MapToLogLevel(validationResult)) << name << " block validation failed due to " << validationResult;
						return false;
					}
				}
			} catch (std::exception const& error) {
				CATAPULT_LOG(warning) << "error validating " << name << " block: " << error.what();
				return false;
			} catch (...) {
				CATAPULT_LOG(warning) << "error validating " << name << " block: unknown error";
				return false;
			}

			return true;
		}

		bool ValidateBlockCosignatures(
				const model::Block& block,
				const chain::CommitteeManager& committeeManager,
				const model::NetworkConfiguration& config,
				chain::HarvesterWeight totalSumOfVotes,
				const chain::Committee& committee) {
			auto numCosignatures = block.CosignaturesCount();
			if (committee.Cosigners.size() + 1u < numCosignatures) {
				CATAPULT_LOG(warning) << "rejecting block, number of cosignatures exceeded committee number";
				return false;
			}

			auto blockProposerWeight = committeeManager.weight(committee.BlockProposer, config);
			auto actualSumOfVotes = blockProposerWeight;
			auto pCosignature = block.CosignaturesPtr();
			for (auto i = 0u; i < numCosignatures; ++i, ++pCosignature) {
				if (committee.Cosigners.find(pCosignature->Signer) == committee.Cosigners.end()) {
					CATAPULT_LOG(warning) << "rejecting block, invalid cosigner " << pCosignature->Signer;
					return false;
				}

				if (!model::VerifyBlockHeaderCosignature(block, *pCosignature)) {
					CATAPULT_LOG(warning) << "rejecting block, cosignature invalid";
					return false;
				}

				committeeManager.add(actualSumOfVotes, committeeManager.weight(pCosignature->Signer, config));
			}

			auto minSumOfVotes = totalSumOfVotes;
			committeeManager.mul(minSumOfVotes, config.CommitteeApproval);
			if (!committeeManager.ge(actualSumOfVotes, minSumOfVotes)) {
				CATAPULT_LOG(warning) << "rejecting block, sum of votes insufficient: " << committeeManager.str(actualSumOfVotes) << " < " << committeeManager.str(minSumOfVotes);
				return false;
			}

			return true;
		}

		bool ValidateBlock(
				WeightedVotingFsm& fsm,
				const model::Block& block,
				bool isProposedBlock,
				extensions::ServiceState& state,
				const model::BlockElementSupplier& lastBlockElementSupplier,
				const std::shared_ptr<thread::IoThreadPool>& pValidatorPool) {
			auto name = (isProposedBlock ? "proposed" : "confirmed");
			auto& committeeData = fsm.committeeData();
			if (!committeeData.isBlockBroadcastEnabled()) {
				CATAPULT_LOG(warning) << "rejecting " << name << " block (broadcast is disabled)";
				return false;
			}

			auto expectedHeight = committeeData.currentBlockHeight();
			if (expectedHeight != block.Height) {
				CATAPULT_LOG(warning) << "rejecting " << name << " block (height " << block.Height << " does not equal to expected height " << expectedHeight << ")";
				if (isProposedBlock) {
					committeeData.setUnexpectedProposedBlockHeight(true);
				} else {
					committeeData.setUnexpectedConfirmedBlockHeight(true);
				}
				return false;
			}

			const auto& committeeManager = state.pluginManager().getCommitteeManager(model::Block::Current_Version);
			auto committee = committeeManager.committee();
			if (committee.Round < 0) {
				CATAPULT_LOG(warning) << "rejecting " << name << " block (committee is not yet selected)";
				return false;
			}

			if (block.round() != committee.Round) {
				CATAPULT_LOG(warning) << "rejecting " << name << " block (round " << block.round() << " invalid, expected " << committee.Round << ")";
				return false;
			}

			if (block.Signer != committee.BlockProposer) {
				CATAPULT_LOG(warning) << "rejecting " << name << " block (signer invalid " << block.Signer << ", expected " << committee.BlockProposer << ")";
				return false;
			}

			if (!model::VerifyBlockHeaderSignature(block)) {
				CATAPULT_LOG(warning) << "rejecting " << name << " block from " << committee.BlockProposer << " (signature invalid)";
				return false;
			}

			if (!isProposedBlock && !ValidateBlockCosignatures(block, committeeManager, state.pluginManager().config(block.Height), committeeData.totalSumOfVotes(), committee))
				return false;

			bool isBlockValid = ValidateBlock(block, isProposedBlock, state, lastBlockElementSupplier, pValidatorPool);

			if (isBlockValid) {
				if (isProposedBlock) {
					committeeData.addValidatedProposedBlockSignature(block.Signature);
				} else {
					committeeData.addValidatedConfirmedBlockSignature(block.Signature);
				}
			}

			return isBlockValid;
		}
	}

	bool ValidateProposedBlock(
			WeightedVotingFsm& fsm,
			const ionet::Packet& packet,
			extensions::ServiceState& state,
			const model::BlockElementSupplier& lastBlockElementSupplier,
			const std::shared_ptr<thread::IoThreadPool>& pValidatorPool) {
		auto pBlock = GetBlockFromPacket(state.pluginManager(), packet);
		if (!pBlock)
			return false;

		if (fsm.committeeData().isProposedBlockValidated(pBlock->Signature))
			return true;

		return ValidateBlock(fsm, *pBlock, true, state, lastBlockElementSupplier, pValidatorPool);
	}

	void PushProposedBlock(
			WeightedVotingFsm& fsm,
			const plugins::PluginManager& pluginManager,
			const ionet::Packet& packet) {
		auto& committeeData = fsm.committeeData();
		if (committeeData.proposedBlock()) {
			CATAPULT_LOG(trace) << "rejecting proposed block, there is one already";
			return;
		}

		auto pBlock = GetBlockFromPacket(pluginManager, packet);
		if (pBlock)
			committeeData.setProposedBlock(pBlock);
	}

	bool ValidateConfirmedBlock(
			WeightedVotingFsm& fsm,
			const ionet::Packet& packet,
			extensions::ServiceState& state,
			const model::BlockElementSupplier& lastBlockElementSupplier,
			const std::shared_ptr<thread::IoThreadPool>& pValidatorPool) {
		auto pBlock = GetBlockFromPacket(state.pluginManager(), packet);
		if (!pBlock)
			return false;

		if (fsm.committeeData().isConfirmedBlockValidated(pBlock->Signature))
			return true;

		return ValidateBlock(fsm, *pBlock, false, state, lastBlockElementSupplier, pValidatorPool);
	}

	void PushConfirmedBlock(WeightedVotingFsm& fsm, const plugins::PluginManager& pluginManager, const ionet::Packet& packet) {
		auto& committeeData = fsm.committeeData();
		if (committeeData.confirmedBlock()) {
			CATAPULT_LOG(trace) << "rejecting confirmed block, there is one already";
			return;
		}

		auto pBlock = GetBlockFromPacket(pluginManager, packet);
		if (pBlock)
			committeeData.setConfirmedBlock(pBlock);
	}

	namespace {
		template<typename TPacket>
		bool ValidateVoteMessages(WeightedVotingFsm& fsm, const ionet::Packet& packet, const std::string& name, bool pushVotes, bool addVotesToBroadcast) {
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

				if (pushVotes) {
					committeeData.addVote(*pMessage);
					CATAPULT_LOG(debug) << "collected " << committeeData.votes(pMessage->Type).size() << " " << name << "(s)";
				} else if (addVotesToBroadcast) {
					committeeData.addVoteToBroadcast(*pMessage);
					CATAPULT_LOG(debug) << "received " << name << " to broadcast from " << pMessage->BlockCosignature.Signer;
				}
			}

			return true;
		}
	}

	bool ValidatePrevoteMessages(WeightedVotingFsm& fsm, const ionet::Packet& packet) {
		return ValidateVoteMessages<PushPrevoteMessagesRequest>(fsm, packet, "prevote message", false, false);
	}

	void PushPrevoteMessages(WeightedVotingFsm& fsm, const ionet::Packet& packet) {
		ValidateVoteMessages<PushPrevoteMessagesRequest>(fsm, packet, "prevote message", true, false);
	}

	bool ValidatePrecommitMessages(WeightedVotingFsm& fsm, const ionet::Packet& packet) {
		return ValidateVoteMessages<PushPrecommitMessagesRequest>(fsm, packet, "precommit message", false, true);
	}

	void PushPrecommitMessages(WeightedVotingFsm& fsm, const ionet::Packet& packet) {
		ValidateVoteMessages<PushPrecommitMessagesRequest>(fsm, packet, "precommit message", true, false);
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
				CATAPULT_LOG(warning) << "rejecting invalid request: " << packet;
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