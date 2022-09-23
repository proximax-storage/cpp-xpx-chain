/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "WeightedVotingFsm.h"
#include "api/RemoteNodeApi.h"
#include "utils/WeightedVotingUtils.h"
#include "catapult/api/RemoteChainApi.h"
#include "catapult/extensions/LocalNodeChainScore.h"
#include "catapult/extensions/PluginUtils.h"
#include "catapult/chain/BlockDifficultyScorer.h"
#include "catapult/chain/ChainUtils.h"
#include "catapult/crypto/Signer.h"
#include "catapult/extensions/ExecutionConfigurationFactory.h"
#include "catapult/harvesting_core/UnlockedAccounts.h"
#include "catapult/model/BlockUtils.h"
#include "catapult/utils/StackLogger.h"
#include "catapult/validators/AggregateEntityValidator.h"

namespace catapult { namespace fastfinality {

	namespace {
		constexpr auto CommitteePhaseCount = 4u;

		bool ApprovalRatingSufficient(
				const double approvalRating,
				const double totalRating,
				const model::NetworkConfiguration& config) {
			return approvalRating / totalRating >= config.CommitteeEndSyncApproval;
		}

		void DelayAction(
				std::shared_ptr<WeightedVotingFsm> pFsmShared,
				boost::asio::system_timer& timer,
				utils::TimePoint expirationTime,
				action callback,
				action cancelledCallback = [](){}) {
			auto& committeeData = pFsmShared->committeeData();
			std::weak_ptr<WeightedVotingFsm> pFsmWeak = pFsmShared;
			timer.expires_at(expirationTime);
			timer.async_wait([pFsmWeak, callback, cancelledCallback](const boost::system::error_code& ec) {
				TRY_GET_FSM()

				if (ec) {
					if (ec == boost::asio::error::operation_aborted) {
						if (!pFsmShared->stopped())
							cancelledCallback();
						return;
					}

					CATAPULT_THROW_EXCEPTION(boost::system::system_error(ec));
				}

				callback();
			});
		}

		void DelayAction(
				std::shared_ptr<WeightedVotingFsm> pFsmShared,
				boost::asio::system_timer& timer,
				uint64_t delay,
				action callback,
				action cancelledCallback = [](){}) {
			auto expirationTime = pFsmShared->committeeData().committeeStage().RoundStart + std::chrono::milliseconds(delay);
			DelayAction(pFsmShared, timer, expirationTime, callback, cancelledCallback);
		}
	}

	action CreateDefaultCheckLocalChainAction(
			std::weak_ptr<WeightedVotingFsm> pFsmWeak,
			const RemoteNodeStateRetriever& retriever,
			const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder,
			const model::BlockElementSupplier& lastBlockElementSupplier,
			const std::function<uint64_t (const Key&)>& importanceGetter,
			const chain::CommitteeManager& committeeManager) {
		return [pFsmWeak, retriever, pConfigHolder, lastBlockElementSupplier, importanceGetter, &committeeManager]() {
			TRY_GET_FSM()
			
			pFsmShared->setNodeWorkState(NodeWorkState::Synchronizing);
			pFsmShared->resetChainSyncData();
			pFsmShared->resetCommitteeData();

			std::vector<RemoteNodeState> remoteNodeStates;
			{
				remoteNodeStates = retriever().get();
			}

		  	const auto& config = pConfigHolder->Config().Network;
		  	if (remoteNodeStates.empty()) {
				DelayAction(pFsmShared, pFsmShared->timer(), config.CommitteeChainHeightRequestInterval.millis(), [pFsmWeak] {
					TRY_GET_FSM()

					pFsmShared->processEvent(NetworkHeightDetectionFailure{});
				});
				return;
			}

			std::sort(remoteNodeStates.begin(), remoteNodeStates.end(), [](auto a, auto b) {
				return (a.Height == b.Height ? a.BlockHash > b.BlockHash : a.Height > b.Height);
			});

			auto& chainSyncData = pFsmShared->chainSyncData();
			chainSyncData.NetworkHeight = remoteNodeStates.begin()->Height;
			chainSyncData.LocalHeight = lastBlockElementSupplier()->Block.Height;

			if (chainSyncData.NetworkHeight < chainSyncData.LocalHeight) {

				pFsmShared->processEvent(NetworkHeightLessThanLocal{});

			} else if (chainSyncData.NetworkHeight > chainSyncData.LocalHeight) {

				std::map<Hash256, std::pair<uint64_t, std::vector<Key>>> hashKeys;

				for (const auto& state : remoteNodeStates) {
					if (state.Height < chainSyncData.NetworkHeight) {
						break;
					}

					auto& pair = hashKeys[state.BlockHash];
					pair.first += importanceGetter(state.NodeKey);
					for (const auto& key : state.HarvesterKeys) {
						pair.first += importanceGetter(key);
					}
					pair.second.push_back(state.NodeKey);
				}

				std::map<uint64_t, std::vector<Key>> importanceKeys;
				for (const auto& pair : hashKeys)
					importanceKeys[pair.second.first] = pair.second.second;

				chainSyncData.NodeIdentityKeys = std::move(importanceKeys.begin()->second);
				pFsmShared->processEvent(NetworkHeightGreaterThanLocal{});

			} else {

				double approvalRating = 0;
				double totalRating = 0;
				const auto& localBlockHash = lastBlockElementSupplier()->EntityHash;

				for (const auto& state : remoteNodeStates) {
					uint64_t importance = 0;
					for (const auto& key : state.HarvesterKeys) {
						importance += importanceGetter(key);
					}
					double alpha;
					if (state.BlockHash != localBlockHash) {
						alpha = 0;
					} else if (state.NodeWorkState != NodeWorkState::Running) {
						alpha = config.CommitteeNotRunningContribution;
					} else {
						alpha = 1;
					}
					double importance_log = std::log10((double) (importance + config.CommitteeBaseTotalImportance));
					approvalRating += alpha * importance_log;
					totalRating += importance_log;
				}

				if (ApprovalRatingSufficient(approvalRating, totalRating, config)) {
					pFsmShared->processEvent(NetworkHeightEqualToLocal{});
				} else {
					DelayAction(pFsmShared, pFsmShared->timer(), config.CommitteeChainHeightRequestInterval.millis(), [pFsmWeak] {
						TRY_GET_FSM()

						pFsmShared->processEvent(NetworkHeightDetectionFailure{});
					});
				}
			}
		};
	}

	action CreateDefaultResetLocalChainAction() {
		return []() {
			CATAPULT_THROW_RUNTIME_ERROR("local chain is invalid and needs to be reset");
		};
	}

	namespace {
		bool ValidateBlockCosignatures(
				const std::shared_ptr<model::Block>& pBlock,
				const chain::CommitteeManager& committeeManager,
				double committeeApproval) {
			const auto& committee = committeeManager.committee();
			if (pBlock->Signer != committee.BlockProposer) {
				CATAPULT_LOG(warning) << "rejecting block, signer " << pBlock->Signer
					<< " invalid, expected " << committee.BlockProposer;
				return false;
			}

			auto numCosignatures = pBlock->CosignaturesCount();
			if (committee.Cosigners.size() + 1u < numCosignatures) {
				CATAPULT_LOG(warning) << "rejecting block, number of cosignatures exceeded committee number";
				return false;
			}

			auto blockProposerWeight = committeeManager.weight(committee.BlockProposer);
			auto actualSumOfVotes = blockProposerWeight;
			auto pCosignature = pBlock->CosignaturesPtr();
			for (auto i = 0u; i < numCosignatures; ++i, ++pCosignature) {
				if (committee.Cosigners.find(pCosignature->Signer) == committee.Cosigners.end()) {
					CATAPULT_LOG(warning) << "rejecting block, invalid cosigner " << pCosignature->Signer;
					return false;
				}
				actualSumOfVotes += committeeManager.weight(pCosignature->Signer);
			}

			auto totalSumOfVotes = blockProposerWeight;
			for (const auto& cosigner : committee.Cosigners)
				totalSumOfVotes += committeeManager.weight(cosigner);

			if (actualSumOfVotes < committeeApproval * totalSumOfVotes) {
				CATAPULT_LOG(warning) << "rejecting block, sum of votes insufficient";
				return false;
			}

			return true;
		}
	}

	action CreateDefaultDownloadBlocksAction(
			std::weak_ptr<WeightedVotingFsm> pFsmWeak,
			extensions::ServiceState& state,
			consumer<model::BlockRange&&, const disruptor::ProcessingCompleteFunc&> rangeConsumer,
			chain::CommitteeManager& committeeManager) {
		return [pFsmWeak, &state, rangeConsumer, &committeeManager]() {
			TRY_GET_FSM()

			const auto& config = state.config();
			auto syncTimeout = config.Node.SyncTimeout;
			auto maxBlocksPerSyncAttempt = config.Node.MaxBlocksPerSyncAttempt;
			auto committeeApproval = config.Network.CommitteeApproval;
			const auto& chainSyncData = pFsmShared->chainSyncData();
			auto startHeight = chainSyncData.LocalHeight + Height(1);
			auto targetHeight = std::min(chainSyncData.NetworkHeight, chainSyncData.LocalHeight + Height(maxBlocksPerSyncAttempt));
			api::BlocksFromOptions blocksFromOptions{
				utils::checked_cast<uint64_t, uint32_t>(targetHeight.unwrap() - chainSyncData.LocalHeight.unwrap()),
				config.Node.MaxChainBytesPerSyncAttempt.bytes32()
			};

			for (const auto& identityKey : chainSyncData.NodeIdentityKeys) {
				std::vector<std::shared_ptr<model::Block>> blocks;
				{
					auto packetIoPair = state.packetIoPickers().pickMatching(syncTimeout, identityKey);
					if (!packetIoPair)
						continue;

					auto pRemoteChainApi = api::CreateRemoteChainApi(
						*packetIoPair.io(),
						identityKey,
						state.pluginManager().transactionRegistry());

					try {
						auto blockRange = pRemoteChainApi->blocksFrom(startHeight, blocksFromOptions).get();
						blocks = model::EntityRange<model::Block>::ExtractEntitiesFromRange(std::move(blockRange));
					} catch (...) {
						continue;
					}
				}

				bool success = false;
				for (const auto& pBlock : blocks) {
					CATAPULT_LOG(debug) << "committee round " << committeeManager.committee().Round << ", block round " << pBlock->round();
					committeeManager.reset();
					while (committeeManager.committee().Round < pBlock->round())
						committeeManager.selectCommittee(config.Network);

					if (ValidateBlockCosignatures(pBlock, committeeManager, committeeApproval)) {
						std::lock_guard<std::mutex> guard(pFsmShared->mutex());

						auto pPromise = std::make_shared<thread::promise<bool>>();
						auto blockHeight = pBlock->Height;
						rangeConsumer(model::BlockRange::FromEntity(pBlock), [pPromise, blockHeight](auto, const auto& result) {
							bool success = (disruptor::CompletionStatus::Aborted != result.CompletionStatus);
							if (success) {
								CATAPULT_LOG(info) << "successfully committed block (height " << blockHeight << ")";
							} else {
								auto validationResult = static_cast<validators::ValidationResult>(result.CompletionCode);
								CATAPULT_LOG_LEVEL(MapToLogLevel(validationResult))
									<< "block (height " << blockHeight << ") commit failed due to " << validationResult;
							}

							pPromise->set_value(std::move(success));
						});

						success = pPromise->get_future().get();
					} else {
						success = false;
					}

					if (!success) {
						startHeight = pBlock->Height;
						blocksFromOptions.NumBlocks = utils::checked_cast<uint64_t, uint32_t>(targetHeight.unwrap() - startHeight.unwrap()) + 1u;
						break;
					}
				}

				if (success) {
					pFsmShared->processEvent(DownloadBlocksSucceeded{});
					return;
				}
			}

			pFsmShared->processEvent(DownloadBlocksFailed{});
		};
	}

	namespace {
		void IncreasePhaseTime(uint64_t& phaseTimeMillis, const model::NetworkConfiguration& config) {
			if (1.0 == config.CommitteeTimeAdjustment)
				return;

			auto maxPhaseTimeMillis = config.MaxCommitteePhaseTime.millis();
			if (phaseTimeMillis == maxPhaseTimeMillis)
				return;

			phaseTimeMillis *= config.CommitteeTimeAdjustment;
			if (phaseTimeMillis > maxPhaseTimeMillis)
				phaseTimeMillis = maxPhaseTimeMillis;
		}

		void DecreasePhaseTime(uint64_t& phaseTimeMillis, const model::NetworkConfiguration& config) {
			if (1.0 == config.CommitteeTimeAdjustment)
				return;

			auto minPhaseTimeMillis = config.MinCommitteePhaseTime.millis();
			if (phaseTimeMillis == minPhaseTimeMillis)
				return;

			phaseTimeMillis /= config.CommitteeTimeAdjustment;
			if (phaseTimeMillis < minPhaseTimeMillis)
				phaseTimeMillis = minPhaseTimeMillis;
		}
	}

	action CreateDefaultDetectStageAction(
			std::weak_ptr<WeightedVotingFsm> pFsmWeak,
			const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder,
			const chain::TimeSupplier& timeSupplier,
			const model::BlockElementSupplier& lastBlockElementSupplier,
			chain::CommitteeManager& committeeManager) {
		return [pFsmWeak, pConfigHolder, timeSupplier, lastBlockElementSupplier, &committeeManager]() {
			TRY_GET_FSM()

		  	pFsmShared->resetChainSyncData();
			pFsmShared->setNodeWorkState(NodeWorkState::Running);
			committeeManager.reset();

			auto pLastBlockElement = lastBlockElementSupplier();
			const auto& block = pLastBlockElement->Block;
			const auto& config = pConfigHolder->Config().Network;
			auto phaseTimeMillis = block.committeePhaseTime() ? block.committeePhaseTime() : config.CommitteePhaseTime.millis();

			auto roundStart = block.Timestamp;
			auto currentTime = timeSupplier();
			if (roundStart > currentTime)
				CATAPULT_THROW_RUNTIME_ERROR_2("invalid current time", currentTime, roundStart);

			DecreasePhaseTime(phaseTimeMillis, config);
			auto nextRoundStart = roundStart + Timestamp(CommitteePhaseCount * phaseTimeMillis);
			committeeManager.selectCommittee(config);

			while (nextRoundStart < timeSupplier()) {
				roundStart = nextRoundStart;
				IncreasePhaseTime(phaseTimeMillis, config);
				nextRoundStart = nextRoundStart + Timestamp(CommitteePhaseCount * phaseTimeMillis);

				committeeManager.selectCommittee(config);
			}

			CommitteeStage stage {
				committeeManager.committee().Round,
				(timeSupplier() < roundStart +
					Timestamp(phaseTimeMillis - config.CommitteeRequestInterval.millis() - config.CommitteeSilenceInterval.millis())) ?
						CommitteePhase::Propose : CommitteePhase::Prevote,
				utils::ToTimePoint(roundStart),
				phaseTimeMillis
			};

			CATAPULT_LOG(debug) << "phase " << stage.Phase << ", phase time " << phaseTimeMillis << "ms, round " << stage.Round;
			pFsmShared->committeeData().setCommitteeStage(stage);
			pFsmShared->processEvent(StageDetectionSucceeded{});
		};
	}

	action CreateDefaultSelectCommitteeAction(
			std::weak_ptr<WeightedVotingFsm> pFsmWeak,
			chain::CommitteeManager& committeeManager,
			const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder,
			const chain::TimeSupplier& timeSupplier) {
		return [pFsmWeak, &committeeManager, pConfigHolder, timeSupplier]() {
			TRY_GET_FSM()

			auto& committeeData = pFsmShared->committeeData();
			auto stage = committeeData.committeeStage();
			if (CommitteePhase::None == stage.Phase)
				CATAPULT_THROW_RUNTIME_ERROR("committee phase is not set");

			if (committeeManager.committee().Round > stage.Round)
				CATAPULT_THROW_RUNTIME_ERROR_2("invalid committee round", committeeManager.committee().Round, stage.Round);

			const auto& config = pConfigHolder->Config().Network;
			while (committeeManager.committee().Round < stage.Round)
				committeeManager.selectCommittee(config);

			const auto& committee = committeeManager.committee();
			auto accounts = committeeData.unlockedAccounts()->view();

			auto blockProposerIter = std::find_if(accounts.begin(), accounts.end(), [&committee](const auto& keyPair) {
				return (committee.BlockProposer == keyPair.publicKey());
			});
			bool isBlockProposer = (blockProposerIter != accounts.end());
			committeeData.setBlockProposer(isBlockProposer ? &(*blockProposerIter) : nullptr);

			const auto& cosigners = committee.Cosigners;
			auto& localCommittee = committeeData.localCommittee();
			localCommittee.clear();
			if (isBlockProposer)
				localCommittee.insert(&(*blockProposerIter));

			std::for_each(accounts.begin(), accounts.end(), [&cosigners, &localCommittee](const auto& keyPair) {
				auto cosignerIter = std::find_if(cosigners.begin(), cosigners.end(), [&keyPair](const auto& cosigner) {
					return (cosigner == keyPair.publicKey());
				});
				if (cosignerIter != cosigners.end())
					localCommittee.insert(&keyPair);
			});

			double totalSumOfVotes = committeeManager.weight(committee.BlockProposer);
			for (const auto& cosigner : committee.Cosigners)
				totalSumOfVotes += committeeManager.weight(cosigner);
			committeeData.setTotalSumOfVotes(totalSumOfVotes);

			pFsmShared->processEvent(CommitteeSelectionResult{ isBlockProposer, stage.Phase });
		};
	}

	namespace {
		struct NextBlockContext {
		public:
			explicit NextBlockContext(const model::BlockElement& parentBlockElement, Timestamp nextTimestamp)
					: ParentBlock(parentBlockElement.Block)
					, ParentContext(parentBlockElement)
					, Timestamp(std::move(nextTimestamp))
					, Height(ParentBlock.Height + catapult::Height(1))
					, BlockTime(utils::TimeSpan::FromDifference(Timestamp, ParentBlock.Timestamp))
			{}

		public:
			const model::Block& ParentBlock;
			model::PreviousBlockContext ParentContext;
			catapult::Timestamp Timestamp;
			catapult::Height Height;
			utils::TimeSpan BlockTime;
			catapult::Difficulty Difficulty;

		public:
			bool tryCalculateDifficulty(const cache::BlockDifficultyCache& cache, const model::NetworkConfiguration& config) {
				return chain::TryCalculateDifficulty(cache, state::BlockDifficultyInfo(Height, Timestamp, Difficulty), config, Difficulty);
			}
		};
	}

	action CreateDefaultProposeBlockAction(
			std::weak_ptr<WeightedVotingFsm> pFsmWeak,
			const cache::CatapultCache& cache,
			const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder,
			const harvesting::BlockGenerator& blockGenerator,
			const model::BlockElementSupplier& lastBlockElementSupplier,
			const extensions::PacketPayloadSink& packetPayloadSink) {
		return [pFsmWeak, &cache, pConfigHolder, blockGenerator, lastBlockElementSupplier, packetPayloadSink]() {
			TRY_GET_FSM()

			auto& committeeData = pFsmShared->committeeData();
			committeeData.setProposedBlock(nullptr);
			auto committeeStage = committeeData.committeeStage();
			NextBlockContext context(*lastBlockElementSupplier(), utils::FromTimePoint(committeeStage.RoundStart));
			const auto& config = pConfigHolder->Config(context.Height);
			if (!context.tryCalculateDifficulty(cache.sub<cache::BlockDifficultyCache>(), config.Network)) {
				CATAPULT_LOG(debug) << "skipping block propose attempt due to error calculating difficulty";
				pFsmShared->processEvent(BlockProposingFailed{});
				return;
			}

			utils::StackLogger stackLogger("generating candidate block", utils::LogLevel::Debug);
			auto pBlockHeader = model::CreateBlock(context.ParentContext, config.Immutable.NetworkIdentifier, committeeData.blockProposer()->publicKey(), {});
			pBlockHeader->Difficulty = context.Difficulty;
			pBlockHeader->Timestamp = context.Timestamp;
			pBlockHeader->Beneficiary = committeeData.beneficiary();
			pBlockHeader->setRound(committeeStage.Round);
			pBlockHeader->setCommitteePhaseTime(committeeStage.PhaseTimeMillis);
			auto pBlock = utils::UniqueToShared(blockGenerator(*pBlockHeader, config.Network.MaxTransactionsPerBlock));
			if (pBlock) {
				model::SignBlockHeader(*committeeData.blockProposer(), *pBlock);
				committeeData.setProposedBlock(pBlock);
				packetPayloadSink(ionet::PacketPayloadFactory::FromEntity(ionet::PacketType::Push_Proposed_Block, pBlock));

				pFsmShared->processEvent(BlockProposingSucceeded{});
			} else {
				pFsmShared->processEvent(BlockProposingFailed{});
			}
		};
	}

	namespace {
		auto GetPhaseEndTimeMillis(const CommitteeStage& stage) {
			switch (stage.Phase) {
				case CommitteePhase::Propose:
					return stage.PhaseTimeMillis;
				case CommitteePhase::Prevote:
					return 2 * stage.PhaseTimeMillis;
				case CommitteePhase::Precommit:
					return 3 * stage.PhaseTimeMillis;
				case CommitteePhase::Commit:
					return 4 * stage.PhaseTimeMillis;
				default:
					CATAPULT_THROW_RUNTIME_ERROR_1("invalid committee phase", stage.Phase);
			}
		}
	}

	action CreateDefaultRequestProposalAction(
			std::weak_ptr<WeightedVotingFsm> pFsmWeak,
			extensions::ServiceState& state) {
		return [pFsmWeak, &state]() {
			TRY_GET_FSM()

			auto& committeeData = pFsmShared->committeeData();
			if (committeeData.proposedBlock()) {
				pFsmShared->processEvent(ProposalReceived{});
				return;
			}

			const auto& config = state.config().Network;
			auto stage = committeeData.committeeStage();
			auto requestInterval = std::chrono::milliseconds(config.CommitteeRequestInterval.millis());
			auto requestStartTime = stage.RoundStart + requestInterval;
			if (utils::ToTimePoint(utils::NetworkTime()) < requestStartTime) {
				DelayAction(pFsmShared, pFsmShared->timer(), requestStartTime, [pFsmWeak] {
					TRY_GET_FSM()

					pFsmShared->processEvent(ProposalRequest{});
				});
			}

			auto timeout = utils::TimeSpan::FromSeconds(60);
			auto packetIoPairs = pFsmShared->packetIoPickers().pickMultiple(timeout);
			CATAPULT_LOG(debug) << "found " << packetIoPairs.size() << " peer(s) for pulling proposed block";
			for (const auto& packetIoPair : packetIoPairs) {
				auto pRemoteApi = CreateRemoteNodeApi(*packetIoPair.io(), state.pluginManager().transactionRegistry());
				std::shared_ptr<model::Block> pProposedBlock;

				try {
					pProposedBlock = pRemoteApi->proposedBlock().get();
				} catch (...) {}

				if (pProposedBlock) {
					committeeData.setProposedBlock(pProposedBlock);
					pFsmShared->processEvent(ProposalReceived{});
					return;
				}
			}

			auto phaseEndTime = stage.RoundStart + std::chrono::milliseconds(GetPhaseEndTimeMillis(stage));
			auto currentTime = utils::ToTimePoint(utils::NetworkTime());
			if (currentTime >= phaseEndTime - std::chrono::milliseconds(config.CommitteeSilenceInterval.millis())) {
				DelayAction(pFsmShared, pFsmShared->timer(), phaseEndTime, [pFsmWeak] {
					TRY_GET_FSM()

					pFsmShared->processEvent(ProposalNotReceived{});
				});
				return;
			}

			DelayAction(pFsmShared, pFsmShared->timer(), currentTime + requestInterval, [pFsmWeak] {
				TRY_GET_FSM()

				pFsmShared->processEvent(ProposalRequest{});
			});
		};
	}

	namespace {
		void SetPhase(std::shared_ptr<WeightedVotingFsm> pFsmShared, CommitteePhase phase) {
			auto& committeeData = pFsmShared->committeeData();
			auto stage = committeeData.committeeStage();
			stage.Phase = phase;
			committeeData.setCommitteeStage(stage);
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
			blockConsumers.push_back(consumers::CreateBlockChainCheckConsumer(
				state.config().Node.MaxBlocksPerSyncAttempt,
				state.pluginManager().configHolder(),
				state.timeSupplier()));
			blockConsumers.push_back(consumers::CreateBlockStatelessValidationConsumer(
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

		bool ValidateProposedBlock(
				std::shared_ptr<model::Block> pProposedBlock,
				extensions::ServiceState& state,
				const model::BlockElementSupplier& lastBlockElementSupplier,
				const std::shared_ptr<thread::IoThreadPool>& pValidatorPool) {
			auto blockConsumers = CreateBlockConsumers(state, lastBlockElementSupplier, pValidatorPool);

			std::vector<model::BlockElement> blocks{ model::BlockElement(*pProposedBlock) };
			for (const auto& blockConsumer : blockConsumers) {
				auto result = blockConsumer(blocks);
				if (disruptor::CompletionStatus::Aborted == result.CompletionStatus) {
					auto validationResult = static_cast<validators::ValidationResult>(result.CompletionCode);
					CATAPULT_LOG_LEVEL(MapToLogLevel(validationResult))
						<< "proposed block validation failed due to " << validationResult;
					return false;
				}
			}

			return true;
		}
	}

	action CreateDefaultValidateProposalAction(
			std::weak_ptr<WeightedVotingFsm> pFsmWeak,
			extensions::ServiceState& state,
			const model::BlockElementSupplier& lastBlockElementSupplier,
			const std::shared_ptr<thread::IoThreadPool>& pValidatorPool) {
		return [pFsmWeak, &state, lastBlockElementSupplier, pValidatorPool]() {
			TRY_GET_FSM()

			auto& committeeData = pFsmShared->committeeData();
			auto pProposedBlock = committeeData.proposedBlock();
			if (!pProposedBlock)
				CATAPULT_THROW_RUNTIME_ERROR("no proposal to validate");

			if (pProposedBlock->Height > lastBlockElementSupplier()->Block.Height + Height(1)) {
				pFsmShared->processEvent(UnexpectedBlockHeight{});
				return;
			}

			const auto& committee = state.pluginManager().getCommitteeManager().committee();
			if (pProposedBlock->Signer != committee.BlockProposer) {
				CATAPULT_LOG(warning) << "rejecting proposal, signer " << pProposedBlock->Signer
					<< " invalid, expected " << committee.BlockProposer;
				committeeData.setProposedBlock(nullptr);
				pFsmShared->processEvent(ProposalInvalid{});
				return;
			}

			if (pProposedBlock->round() != committee.Round) {
				CATAPULT_LOG(warning) << "rejecting proposal, round " << pProposedBlock->round()
					<< " invalid, expected " << committee.Round;
				committeeData.setProposedBlock(nullptr);
				pFsmShared->processEvent(ProposalInvalid{});
				return;
			}

			if (ValidateProposedBlock(pProposedBlock, state, lastBlockElementSupplier, pValidatorPool)) {
				pFsmShared->processEvent(ProposalValid{});
			} else {
				committeeData.setProposedBlock(nullptr);
				pFsmShared->processEvent(ProposalInvalid{});
			}
		};
	}

	action CreateDefaultWaitForProposalPhaseEndAction(
			std::weak_ptr<WeightedVotingFsm> pFsmWeak,
			const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder) {
		return [pFsmWeak, pConfigHolder]() {
			TRY_GET_FSM()

			auto phaseEndTimeMillis = GetPhaseEndTimeMillis(pFsmShared->committeeData().committeeStage());
			DelayAction(pFsmShared, pFsmShared->timer(), phaseEndTimeMillis, [pFsmWeak] {
				TRY_GET_FSM()

				pFsmShared->processEvent(ProposalPhaseEnded{});
			});
		};
	}

	namespace {
		bool IsSumOfVotesSufficient(
				const VoteMap& votes,
				double committeeApproval,
				double totalSumOfVotes,
				const chain::CommitteeManager& committeeManager) {
			double sum = 0.0;
			for (const auto& pair : votes)
				sum += committeeManager.weight(pair.first);

			bool sumOfVotesSufficient = sum >= committeeApproval * totalSumOfVotes;
			if (!sumOfVotesSufficient)
				CATAPULT_LOG(debug) << "sum of votes insufficient: " << sum << " < " << committeeApproval << " * " << totalSumOfVotes << ", vote count " << votes.size();
			return sumOfVotesSufficient;
		}

		action CreateRequestVotesAction(
				std::weak_ptr<WeightedVotingFsm> pFsmWeak,
				const plugins::PluginManager& pluginManager,
				const std::string& name,
				CommitteePhase committeePhase,
				CommitteeMessageType messageType,
				std::function<std::vector<CommitteeMessage> (const RemoteNodeApi&)> remoteVoteSupplier,
				std::function<VoteMap (const CommitteeData&)> voteSupplier,
				consumer<bool> onVoteResult) {
			return [pFsmWeak, &pluginManager, name, committeePhase, messageType, remoteVoteSupplier, voteSupplier, onVoteResult]() {
				TRY_GET_FSM()

				const auto& committeeManager = pluginManager.getCommitteeManager();
				const auto& config = pluginManager.configHolder()->Config().Network;
				auto& committeeData = pFsmShared->committeeData();
				auto stage = committeeData.committeeStage();
				auto timeout = utils::TimeSpan::FromSeconds(60);
				auto packetIoPairs = pFsmShared->packetIoPickers().pickMultiple(timeout);
				CATAPULT_LOG(debug) << "found " << packetIoPairs.size() << " peer(s) for pulling " << name << "s";
				for (const auto& packetIoPair : packetIoPairs) {
					auto pRemoteApi = CreateRemoteNodeApiWithoutRegistry(*packetIoPair.io());
					auto messages = remoteVoteSupplier(*pRemoteApi);
					for (const auto& message : messages) {
						if (message.Type != messageType) {
							CATAPULT_LOG(warning) << "rejecting " << name << ", invalid message type " << utils::to_underlying_type(message.Type);
							continue;
						}

						const auto& signer = message.BlockCosignature.Signer;
						if (committeeData.hasVote(signer, message.Type)) {
							CATAPULT_LOG(trace) << "already has vote " << signer << ", phase " << committeePhase;
							continue;
						}

						auto pProposedBlock = committeeData.proposedBlock();
						if (!pProposedBlock) {
							CATAPULT_LOG(warning) << "rejecting " << name << ", no proposed block" << ", phase " << committeePhase;
							continue;
						}

						if (message.BlockHash != committeeData.proposedBlockHash()) {
							CATAPULT_LOG(warning) << "rejecting " << name << ", block hash invalid";
							continue;
						}

						const auto& cosignature = message.BlockCosignature;
						if (!crypto::Verify(cosignature.Signer, CommitteeMessageDataBuffer(message), message.MessageSignature)) {
							CATAPULT_LOG(warning) << "rejecting " << name << ", signature invalid";
							continue;
						}

						if (!model::VerifyBlockHeaderCosignature(*pProposedBlock, cosignature)) {
							CATAPULT_LOG(warning) << "rejecting " << name << ", block signature invalid";
							continue;
						}

						committeeData.addVote(message);
					}

					auto voteCount = utils::checked_cast<size_t, uint32_t>(voteSupplier(committeeData).size());
					CATAPULT_LOG(debug) << "collected " << voteCount << " " << committeePhase << " votes out of " << config.CommitteeSize;
					if (voteCount == config.CommitteeSize) {
						DelayAction(pFsmShared, pFsmShared->timer(), GetPhaseEndTimeMillis(stage),
							[pFsmWeak, committeeApproval = config.CommitteeApproval, &committeeManager, voteSupplier, onVoteResult] {
								TRY_GET_FSM()

								const auto& committeeData = pFsmShared->committeeData();
								bool sumOfVotesSufficient = IsSumOfVotesSufficient(
									voteSupplier(committeeData),
									committeeApproval,
									committeeData.totalSumOfVotes(),
									committeeManager);
								onVoteResult(sumOfVotesSufficient);
							}
						);

						return;
					}
				}

				auto phaseEndTime = stage.RoundStart + std::chrono::milliseconds(GetPhaseEndTimeMillis(stage));
				auto currentTime = utils::ToTimePoint(utils::NetworkTime());
				if (currentTime >= phaseEndTime - std::chrono::milliseconds(config.CommitteeSilenceInterval.millis())) {
					DelayAction(pFsmShared, pFsmShared->timer(), phaseEndTime,
							[pFsmWeak, committeeApproval = config.CommitteeApproval, &committeeManager, voteSupplier, onVoteResult] {
						TRY_GET_FSM()

						const auto& committeeData = pFsmShared->committeeData();
						bool sumOfVotesSufficient = IsSumOfVotesSufficient(
							voteSupplier(committeeData),
							committeeApproval,
							committeeData.totalSumOfVotes(),
							committeeManager);
						onVoteResult(sumOfVotesSufficient);
					});
					return;
				}

				auto requestInterval = std::chrono::milliseconds(config.CommitteeRequestInterval.millis());
				DelayAction(pFsmShared, pFsmShared->timer(), currentTime + requestInterval, [pFsmWeak, name] {
					TRY_GET_FSM()

					pFsmShared->processEvent(VoteRequest{});
				});
			};
		}
	}

	action CreateDefaultRequestPrevotesAction(
			std::weak_ptr<WeightedVotingFsm> pFsmWeak,
			const plugins::PluginManager& pluginManager) {
		return CreateRequestVotesAction(
			pFsmWeak,
			pluginManager,
			"prevote message",
			CommitteePhase::Prevote,
			CommitteeMessageType::Prevote,
			[](auto& remoteApi) {
				std::vector<CommitteeMessage> prevotes;
				try {
					prevotes = remoteApi.prevotes().get();
				} catch (...) {}
				return prevotes;
			},
			[](auto& committeeData) {
				return committeeData.prevotes();
			},
			[pFsmWeak] (bool sumOfPrevotesSufficient) {
				TRY_GET_FSM()

				SetPhase(pFsmShared, CommitteePhase::Precommit);

				pFsmShared->committeeData().setSumOfPrevotesSufficient(sumOfPrevotesSufficient);
				if (sumOfPrevotesSufficient) {
					pFsmShared->processEvent(SumOfPrevotesSufficient{});
				} else {
					pFsmShared->processEvent(SumOfPrevotesInsufficient{});
				}
			});
	}

	action CreateDefaultRequestPrecommitsAction(
			std::weak_ptr<WeightedVotingFsm> pFsmWeak,
			const plugins::PluginManager& pluginManager) {
		return CreateRequestVotesAction(
			pFsmWeak,
			pluginManager,
			"precommit message",
			CommitteePhase::Precommit,
			CommitteeMessageType::Precommit,
			[](auto& remoteApi) {
				std::vector<CommitteeMessage> precommits;
				try {
					precommits = remoteApi.precommits().get();
				} catch (...) {}
				return precommits;
			},
			[](auto& committeeData) {
				return committeeData.precommits();
			},
			[pFsmWeak] (bool sumOfPrecommitsSufficient) {
				TRY_GET_FSM()

				SetPhase(pFsmShared, CommitteePhase::Commit);

				if (sumOfPrecommitsSufficient && pFsmShared->committeeData().sumOfPrevotesSufficient()) {
					pFsmShared->processEvent(SumOfPrecommitsSufficient{});
				} else {
					pFsmShared->processEvent(SumOfPrecommitsInsufficient{});
				}
			});
	}

	namespace {
		template<typename TPacket, CommitteeMessageType MessageType, CommitteePhase Phase>
		action CreateAddVoteAction(
				std::weak_ptr<WeightedVotingFsm> pFsmWeak,
				const extensions::PacketPayloadSink& packetPayloadSink) {
			return [pFsmWeak, packetPayloadSink]() {
				TRY_GET_FSM()

				SetPhase(pFsmShared, Phase);

				auto& committeeData = pFsmShared->committeeData();
				auto pProposedBlock = committeeData.proposedBlock();
				if (!pProposedBlock)
					CATAPULT_THROW_RUNTIME_ERROR_1("add vote failed, no proposed block", Phase);

				for (const auto* pKeyPair : committeeData.localCommittee()) {
					CommitteeMessage message;
					message.Type = MessageType;
					message.BlockHash = committeeData.proposedBlockHash();
					auto& cosignature = message.BlockCosignature;
					cosignature.Signer = pKeyPair->publicKey();
					model::CosignBlockHeader(*pKeyPair, *pProposedBlock, cosignature.Signature);
					crypto::Sign(*pKeyPair, CommitteeMessageDataBuffer(message), message.MessageSignature);
					committeeData.addVote(message);
				}

				auto votes = committeeData.votes(MessageType);
				CATAPULT_LOG(debug) << "added " << votes.size() << " " << Phase << " votes";
				if (votes.size() == 0)
					return;

				auto pPacket = ionet::CreateSharedPacket<TPacket>(utils::checked_cast<size_t, uint32_t>(sizeof(CommitteeMessage) * votes.size()));
				pPacket->MessageCount = utils::checked_cast<size_t, uint8_t>(votes.size());

				auto* pMessage = reinterpret_cast<CommitteeMessage*>(pPacket.get() + 1);
				auto index = 0u;
				for (const auto& pair : votes)
					pMessage[index++] = pair.second;

				packetPayloadSink(ionet::PacketPayload(pPacket));
			};
		}
	}

	action CreateDefaultAddPrevoteAction(std::weak_ptr<WeightedVotingFsm> pFsmWeak, const extensions::PacketPayloadSink& packetPayloadSink) {
		return CreateAddVoteAction<PushPrevoteMessagesRequest, CommitteeMessageType::Prevote, CommitteePhase::Prevote>(pFsmWeak, packetPayloadSink);
	}

	action CreateDefaultAddPrecommitAction(std::weak_ptr<WeightedVotingFsm> pFsmWeak, const extensions::PacketPayloadSink& packetPayloadSink) {
		return CreateAddVoteAction<PushPrecommitMessagesRequest, CommitteeMessageType::Precommit, CommitteePhase::Precommit>(pFsmWeak, packetPayloadSink);
	}

	action CreateDefaultWaitForPrecommitPhaseEndAction(
			std::weak_ptr<WeightedVotingFsm> pFsmWeak,
			const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder) {
		return [pFsmWeak, pConfigHolder]() {
			TRY_GET_FSM()

			SetPhase(pFsmShared, CommitteePhase::Precommit);
			auto phaseEndTimeMillis = GetPhaseEndTimeMillis(pFsmShared->committeeData().committeeStage());
			DelayAction(pFsmShared, pFsmShared->timer(), phaseEndTimeMillis, [pFsmWeak] {
				TRY_GET_FSM()

				SetPhase(pFsmShared, CommitteePhase::Commit);
				pFsmShared->processEvent(PrecommitPhaseEnded{});
			});
		};
	}

	action CreateDefaultUpdateConfirmedBlockAction(
		std::weak_ptr<WeightedVotingFsm> pFsmWeak,
		chain::CommitteeManager& committeeManager) {
		return [pFsmWeak, &committeeManager]() {
			TRY_GET_FSM()

			auto& committeeData = pFsmShared->committeeData();
			auto votes = committeeData.precommits();
			std::vector<model::Cosignature> cosignatures;
			cosignatures.reserve(votes.size());
			const auto& blockProposer = committeeManager.committee().BlockProposer;
			for (const auto &pair : votes) {
				if (pair.first != blockProposer)
					cosignatures.push_back(pair.second.BlockCosignature);
			}
			auto cosignaturesSize = cosignatures.size() * sizeof(model::Cosignature);

			auto pProposedBlock = committeeData.proposedBlock();
			if (!pProposedBlock)
				CATAPULT_THROW_RUNTIME_ERROR("update confirmed block failed, no proposed block");

			auto blockSize = pProposedBlock->Size + cosignaturesSize;
			auto pBlock = utils::MakeSharedWithSize<model::Block>(blockSize);
			std::memcpy(static_cast<void *>(pBlock.get()), pProposedBlock.get(), pProposedBlock->Size);
			pBlock->Size = blockSize;
			std::memcpy(static_cast<void *>(pBlock->CosignaturesPtr()), cosignatures.data(), cosignaturesSize);
			committeeData.setProposedBlock(nullptr);
			committeeData.setConfirmedBlock(pBlock);
		};
	}

	action CreateDefaultCommitConfirmedBlockAction(
			std::weak_ptr<WeightedVotingFsm> pFsmWeak,
			consumer<model::BlockRange&&, const disruptor::ProcessingCompleteFunc&> rangeConsumer,
			const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder,
			chain::CommitteeManager& committeeManager) {
		return [pFsmWeak, rangeConsumer, pConfigHolder, &committeeManager]() {
			TRY_GET_FSM()

			auto& committeeData = pFsmShared->committeeData();
			auto pBlock = committeeData.confirmedBlock();
			if (!pBlock)
				CATAPULT_THROW_RUNTIME_ERROR("commit confirmed block failed, no block");

			auto stage = committeeData.committeeStage();
			const auto& config = pConfigHolder->Config().Network;
			auto requestInterval = std::chrono::milliseconds(config.CommitteeRequestInterval.millis());
			if (stage.Round != pBlock->round() || !ValidateBlockCosignatures(pBlock, committeeManager, config.CommitteeApproval)) {
				committeeData.setConfirmedBlock(nullptr);

				DelayAction(pFsmShared, pFsmShared->timer(), utils::ToTimePoint(utils::NetworkTime()) + requestInterval, [pFsmWeak] {
					TRY_GET_FSM()

					pFsmShared->processEvent(CommitBlockFailed{});
				});
				return;
			}


			// Commit block.
			bool success = false;
			{
				std::lock_guard<std::mutex> guard(pFsmShared->mutex());

				auto pPromise = std::make_shared<thread::promise<bool>>();
				rangeConsumer(model::BlockRange::FromEntity(pBlock), [pPromise](auto, const auto &result) {
					bool success = (disruptor::CompletionStatus::Aborted != result.CompletionStatus);
					if (success) {
						CATAPULT_LOG(info) << "successfully committed confirmed block";
					} else {
						auto validationResult = static_cast<validators::ValidationResult>(result.CompletionCode);
						CATAPULT_LOG_LEVEL(MapToLogLevel(validationResult))
							<< "confirmed block commit failed due to " << validationResult;
					}

					pPromise->set_value(std::move(success));
				});

				success = pPromise->get_future().get();
			}

			if (success) {
				DelayAction(pFsmShared, pFsmShared->timer(), GetPhaseEndTimeMillis(stage), [pFsmWeak] {
					TRY_GET_FSM()

					pFsmShared->processEvent(CommitBlockSucceeded{});
				});
			} else {
				committeeData.setConfirmedBlock(nullptr);

				DelayAction(pFsmShared, pFsmShared->timer(), utils::ToTimePoint(utils::NetworkTime()) + requestInterval, [pFsmWeak] {
					TRY_GET_FSM()

					pFsmShared->processEvent(CommitBlockFailed{});
				});
			}
		};
	}

	action CreateDefaultIncrementRoundAction(
			std::weak_ptr<WeightedVotingFsm> pFsmWeak,
			const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder) {
		return [pFsmWeak, pConfigHolder]() {
			TRY_GET_FSM()

			auto& committeeData = pFsmShared->committeeData();
			auto currentStage = committeeData.committeeStage();
			pFsmShared->resetCommitteeData();

			int64_t nextRound = currentStage.Round + 1;
			CATAPULT_LOG(debug) << "incremented round " << nextRound;
			auto nextRoundStart = currentStage.RoundStart + std::chrono::milliseconds(GetPhaseEndTimeMillis(currentStage));
			uint64_t nextPhaseTimeMillis = currentStage.PhaseTimeMillis;
			IncreasePhaseTime(nextPhaseTimeMillis, pConfigHolder->Config().Network);
			committeeData.setCommitteeStage(CommitteeStage{
				nextRound,
				CommitteePhase::Propose,
				nextRoundStart,
				nextPhaseTimeMillis
			});
		};
	}

	action CreateDefaultResetRoundAction(
			std::weak_ptr<WeightedVotingFsm> pFsmWeak,
			const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder,
			chain::CommitteeManager& committeeManager) {
		return [pFsmWeak, pConfigHolder, &committeeManager]() {
			TRY_GET_FSM()

			auto& committeeData = pFsmShared->committeeData();
			auto currentStage = committeeData.committeeStage();
			pFsmShared->resetCommitteeData();
			committeeManager.reset();

			const auto& config = pConfigHolder->Config().Network;
			auto nextRoundStart = currentStage.RoundStart + std::chrono::milliseconds(GetPhaseEndTimeMillis(currentStage));
			uint64_t nextPhaseTimeMillis = currentStage.PhaseTimeMillis;
			DecreasePhaseTime(nextPhaseTimeMillis, pConfigHolder->Config().Network);
			committeeData.setCommitteeStage(CommitteeStage{
				0u,
				CommitteePhase::Propose,
				nextRoundStart,
				nextPhaseTimeMillis
			});
		};
	}

	action CreateDefaultRequestConfirmedBlockAction(
		std::weak_ptr<WeightedVotingFsm> pFsmWeak,
		extensions::ServiceState& state,
		const model::BlockElementSupplier& lastBlockElementSupplier) {
		return [pFsmWeak, &state, lastBlockElementSupplier]() {
			TRY_GET_FSM()

			auto& committeeData = pFsmShared->committeeData();
			committeeData.setProposedBlock(nullptr);

			if (committeeData.confirmedBlock()) {
				pFsmShared->processEvent(ConfirmedBlockReceived{});
				return;
			}

			auto timeout = utils::TimeSpan::FromSeconds(60);
			auto packetIoPairs = pFsmShared->packetIoPickers().pickMultiple(timeout);
			CATAPULT_LOG(debug) << "found " << packetIoPairs.size() << " peer(s) for pulling confirmed block";
			for (const auto& packetIoPair : packetIoPairs) {
				auto pRemoteApi = CreateRemoteNodeApi(*packetIoPair.io(), state.pluginManager().transactionRegistry());
				std::shared_ptr<model::Block> pBlock;

				try {
					pBlock = pRemoteApi->confirmedBlock().get();
				} catch (...) {}

				if (pBlock) {
					if (pBlock->Height > lastBlockElementSupplier()->Block.Height + Height(1)) {
						pFsmShared->processEvent(UnexpectedBlockHeight{});
					} else {
						pFsmShared->committeeData().setConfirmedBlock(pBlock);
						pFsmShared->processEvent(ConfirmedBlockReceived{});
					}
					return;
				}
			}

			auto stage = pFsmShared->committeeData().committeeStage();
			auto phaseEndTime = stage.RoundStart + std::chrono::milliseconds(GetPhaseEndTimeMillis(stage));
			auto currentTime = utils::ToTimePoint(utils::NetworkTime());
			auto requestInterval = std::chrono::milliseconds(state.config().Network.CommitteeRequestInterval.millis());
			auto silenceInterval = std::chrono::milliseconds(state.config().Network.CommitteeSilenceInterval.millis());
			if (currentTime >= phaseEndTime - silenceInterval - requestInterval) {
				DelayAction(pFsmShared, pFsmShared->timer(), phaseEndTime, [pFsmWeak] {
					TRY_GET_FSM()

					pFsmShared->processEvent(ConfirmedBlockNotReceived{});
				});
				return;
			}

			DelayAction(pFsmShared, pFsmShared->timer(), currentTime + requestInterval, [pFsmWeak] {
				TRY_GET_FSM()

				pFsmShared->processEvent(ConfirmedBlockRequest{});
			});
		};
	}
}}