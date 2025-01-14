/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "FastFinalityHandlers.h"
#include "FastFinalityFsm.h"
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
		constexpr VersionType Block_Version = 7;

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
				extensions::ServiceState& state,
				const model::BlockElementSupplier& lastBlockElementSupplier,
				const std::shared_ptr<thread::IoThreadPool>& pValidatorPool) {
			auto blockConsumers = CreateBlockConsumers(state, lastBlockElementSupplier, pValidatorPool);
			try {
				std::vector<model::BlockElement> blocks{ model::BlockElement(block) };
				for (const auto& blockConsumer : blockConsumers) {
					auto result = blockConsumer(blocks);
					if (disruptor::CompletionStatus::Aborted == result.CompletionStatus) {
						auto validationResult = static_cast<validators::ValidationResult>(result.CompletionCode);
						CATAPULT_LOG(warning) << " block validation failed due to " << validationResult;
						return false;
					}
				}
			} catch (std::exception const& error) {
				CATAPULT_LOG(warning) << "error validating block: " << error.what();
				return false;
			} catch (...) {
				CATAPULT_LOG(warning) << "error validating block: unknown error";
				return false;
			}

			return true;
		}

		bool ValidateBlock(
				FastFinalityFsm& fsm,
				const model::Block& block,
				extensions::ServiceState& state,
				const model::BlockElementSupplier& lastBlockElementSupplier,
				const std::shared_ptr<thread::IoThreadPool>& pValidatorPool) {
			auto& fastFinalityData = fsm.fastFinalityData();
			auto expectedHeight = fastFinalityData.currentBlockHeight();
			if (expectedHeight != block.Height) {
				CATAPULT_LOG(warning) << "rejecting block (height " << block.Height << " does not equal to expected height " << expectedHeight << ")";
				fastFinalityData.setUnexpectedBlockHeight(true);
				return false;
			}

			const auto& committeeManager = state.pluginManager().getCommitteeManager(Block_Version);
			auto committee = committeeManager.committee();
			if (committee.Round < 0) {
				CATAPULT_LOG(warning) << "rejecting block (committee is not yet selected)";
				return false;
			}

			if (block.round() != committee.Round) {
				CATAPULT_LOG(warning) << "rejecting block (round " << block.round() << " invalid, expected " << committee.Round << ")";
				return false;
			}

			if (!committee.validateBlockProposer(block.Signer)) {
				CATAPULT_LOG(warning) << "rejecting block (signer invalid " << block.Signer << ")";
				return false;
			}

			return ValidateBlock(block, state, lastBlockElementSupplier, pValidatorPool);
		}
	}

	bool ValidateBlock(
			FastFinalityFsm& fsm,
			const ionet::Packet& packet,
			const Hash256& payloadHash,
			extensions::ServiceState& state,
			const model::BlockElementSupplier& lastBlockElementSupplier,
			const std::shared_ptr<thread::IoThreadPool>& pValidatorPool) {
		std::lock_guard<std::mutex> guard(fsm.mutex());
		auto& fastFinalityData = fsm.fastFinalityData();
		auto blockHash = fastFinalityData.proposedBlockHash();
		if (blockHash != Hash256()) {
			if (blockHash == payloadHash) {
				CATAPULT_LOG(trace) << "block has already been validated";
				return true;
			} else {
				CATAPULT_LOG(warning) << "rejecting block (differs from validated one)";
				return false;
			}
		}

		auto pBlock = GetBlockFromPacket(state.pluginManager(), packet);
		if (!pBlock)
			return false;
		
		auto isBlockValid = ValidateBlock(fsm, *pBlock, state, lastBlockElementSupplier, pValidatorPool);
		if (isBlockValid)
			fastFinalityData.setProposedBlockHash(payloadHash);

		return isBlockValid;
	}

	void PushBlock(
			FastFinalityFsm& fsm,
			const plugins::PluginManager& pluginManager,
			const ionet::Packet& packet) {
		auto pBlock = GetBlockFromPacket(pluginManager, packet);
		if (pBlock) {
			auto& fastFinalityData = fsm.fastFinalityData();
			fastFinalityData.setBlock(pBlock);
		}
	}

	void RegisterPullRemoteNodeStateHandler(
			const std::weak_ptr<FastFinalityFsm>& pFsmWeak,
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

			auto pResponsePacket = ionet::CreateSharedPacket<RemoteNodeStatePacket>();
			pResponsePacket->Type = ionet::PacketType::Pull_Remote_Node_State_Response;

			auto pLastBlockElement = lastBlockElementSupplier();
			auto localHeight = pLastBlockElement->Block.Height;
			auto targetHeight = std::min(localHeight, pRequest->Height);
			auto pBlockElement = (localHeight == targetHeight) ? pLastBlockElement : blockElementGetter(targetHeight);

			pResponsePacket->Height = pBlockElement->Block.Height;
			pResponsePacket->BlockHash = pBlockElement->EntityHash;
			pResponsePacket->NodeWorkState = pFsmShared->nodeWorkState();
			pResponsePacket->HarvesterKeysCount = 0;

			context.response(ionet::PacketPayload(pResponsePacket));
		});
	}
}}