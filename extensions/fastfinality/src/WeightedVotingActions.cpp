/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "WeightedVotingActions.h"
#include "WeightedVotingFsm.h"
#include "catapult/api/RemoteChainApi.h"
#include "catapult/extensions/LocalNodeChainScore.h"
#include "catapult/extensions/PluginUtils.h"
#include "catapult/chain/BlockDifficultyScorer.h"
#include "catapult/chain/ChainUtils.h"
#include "catapult/crypto/Signer.h"
#include "catapult/extensions/ExecutionConfigurationFactory.h"
#include "catapult/harvesting_core/UnlockedAccounts.h"
#include "catapult/ionet/PacketPayloadFactory.h"
#include "catapult/model/BlockUtils.h"
#include "catapult/utils/StackLogger.h"
#include "catapult/validators/AggregateEntityValidator.h"

namespace catapult { namespace fastfinality {

	namespace {
		constexpr auto CommitteePhaseCount = 4u;

		bool ApprovalImportanceSufficient(
				const uint64_t approvalImportance,
				const uint64_t totalImportance,
				const model::NetworkConfiguration& config) {
			return (double)approvalImportance / (double)totalImportance >= config.CommitteeEndSyncApproval;
		}

		template<typename TValue, typename TValueArray, typename TValueAdapter>
		auto FindMostFrequentValue(const TValueArray& values, const TValueAdapter& adapter) {
			TValue mostFrequentValue;
			uint32_t maxFrequency = 0;
			std::map<TValue, uint32_t> valueFrequencies;
			for (const auto& valueWrapper : values) {
				const auto& value = adapter(valueWrapper);
				auto frequency = ++valueFrequencies[value];
				if (maxFrequency <= frequency) {
					maxFrequency = frequency;
					mostFrequentValue = value;
				}
			}

			return std::make_pair(mostFrequentValue, maxFrequency);
		}

		void DelayAction(
				std::shared_ptr<WeightedVotingFsm> pFsmShared,
				boost::asio::system_timer& timer,
				uint64_t delay,
				action callback,
				action cancelledCallback = [](){}) {
			auto& committeeData = pFsmShared->committeeData();
			std::weak_ptr<WeightedVotingFsm> pFsmWeak = pFsmShared;
			timer.expires_at(committeeData.committeeStage().RoundStart + std::chrono::milliseconds(delay));
			timer.async_wait([pFsmWeak, callback, cancelledCallback, delay](const boost::system::error_code& ec) {
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

		void UpdateConnections(std::shared_ptr<WeightedVotingFsm> pFsmShared) {
			for (const auto& task : pFsmShared->peerConnectionTasks())
				task.Callback();
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

			UpdateConnections(pFsmShared);
			
			pFsmShared->setNodeWorkState(NodeWorkState::Synchronizing);
			pFsmShared->resetChainSyncData();
			pFsmShared->resetCommitteeData();

			std::vector<RemoteNodeState> remoteNodeStates;
			{
				remoteNodeStates = retriever().get();
			}

		  	const auto& config = pConfigHolder->Config().Network;
		  	if (remoteNodeStates.empty()) {
				DelayAction(pFsmShared, pFsmShared->timer(), config.CommitteeRequestInterval.millis(),
					[pFsmWeak] {
						TRY_GET_FSM()
						pFsmShared->processEvent(NetworkHeightDetectionFailure{});
					}
				);
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

				uint64_t approvalImportance = 0;
				uint64_t totalImportance = config.CommitteeBaseTotalImportance;
				const auto& localBlockHash = lastBlockElementSupplier()->EntityHash;

				for (const auto& state : remoteNodeStates) {
					uint64_t importance = 0;
					for (const auto& key : state.HarvesterKeys) {
						importance += importanceGetter(key);
					}
					if (state.NodeWorkState == NodeWorkState::Running && state.BlockHash == localBlockHash) {
						approvalImportance += importance;
					}
					totalImportance += importance;
				}

				if (ApprovalImportanceSufficient(approvalImportance, totalImportance, config)) {
					pFsmShared->processEvent(NetworkHeightEqualToLocal{});
				} else {
					DelayAction(pFsmShared, pFsmShared->timer(), config.CommitteeRequestInterval.millis(),
						[pFsmWeak] {
							TRY_GET_FSM()
							pFsmShared->processEvent(NetworkHeightDetectionFailure{});
						}
					);
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
				auto weight = committeeManager.weight(pCosignature->Signer);
				actualSumOfVotes += weight;
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
					committeeManager.reset();
					while (committeeManager.committee().Round < pBlock->Round)
						committeeManager.selectCommittee(config.Network);

					if (ValidateBlockCosignatures(pBlock, committeeManager, committeeApproval)) {
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
			auto phaseTimeMillis = block.CommitteePhaseTime ? block.CommitteePhaseTime : config.CommitteePhaseTime.millis();

			auto roundStart = block.Timestamp + Timestamp(CommitteePhaseCount * phaseTimeMillis);
			auto currentTime = timeSupplier();
			if (roundStart > currentTime)
				CATAPULT_THROW_RUNTIME_ERROR_2("invalid current time", currentTime, roundStart);

			DecreasePhaseTime(phaseTimeMillis, config);
			auto nextRoundStart = roundStart + Timestamp(CommitteePhaseCount * phaseTimeMillis);
			committeeManager.selectCommittee(config);
			CommitteeStage stage{
				0,
				CommitteePhase::None,
				utils::ToTimePoint(roundStart),
				phaseTimeMillis
			};

			while (nextRoundStart < timeSupplier()) {
				roundStart = nextRoundStart;
				IncreasePhaseTime(phaseTimeMillis, config);
				nextRoundStart = nextRoundStart + Timestamp(CommitteePhaseCount * phaseTimeMillis);

				committeeManager.selectCommittee(config);
				stage.Round++;
				stage.PhaseTimeMillis = phaseTimeMillis;
			}

			stage.RoundStart = utils::ToTimePoint(roundStart);
			stage.Phase = (timeSupplier() < roundStart + Timestamp(stage.PhaseTimeMillis - 3 * config.CommitteeMessageBroadcastInterval.millis())) ?
				CommitteePhase::Propose : CommitteePhase::Prevote;
			CATAPULT_LOG(debug) << "phase " << stage.Phase << ", phase time " << phaseTimeMillis << "ms";
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

			UpdateConnections(pFsmShared);

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
			const model::BlockElementSupplier& lastBlockElementSupplier) {
		return [pFsmWeak, &cache, pConfigHolder, blockGenerator, lastBlockElementSupplier]() {
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
			pBlockHeader->Round = committeeStage.Round;
			pBlockHeader->CommitteePhaseTime = committeeStage.PhaseTimeMillis;
			auto pBlock = utils::UniqueToShared(blockGenerator(*pBlockHeader, config.Network.MaxTransactionsPerBlock));
			if (pBlock) {
				model::SignBlockHeader(*committeeData.blockProposer(), *pBlock);
				committeeData.setProposedBlock(pBlock);

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

	action CreateDefaultWaitForProposalAction(
			std::weak_ptr<WeightedVotingFsm> pFsmWeak) {
		return [pFsmWeak]() {
			TRY_GET_FSM()

			DelayAction(pFsmShared, pFsmShared->proposalWaitTimer(), GetPhaseEndTimeMillis(pFsmShared->committeeData().committeeStage()),
				[pFsmWeak] {
					TRY_GET_FSM()

					pFsmShared->processEvent(ProposalNotReceived{});
				},
				[pFsmWeak] {
					TRY_GET_FSM()

					pFsmShared->processEvent(ProposalReceived{});
				}
			);
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

			if (pProposedBlock->Round != committee.Round) {
				CATAPULT_LOG(warning) << "rejecting proposal, round " << pProposedBlock->Round
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

	namespace {
		void ScheduleBroadcast(
				const std::shared_ptr<boost::asio::system_timer>& pTimer,
				utils::TimePoint endPoint,
				const std::chrono::milliseconds& interval,
				action broadcast) {
			auto expireTime = utils::ToTimePoint(utils::NetworkTime()) + interval;
			if (expireTime >= endPoint)
				return;

			pTimer->expires_at(expireTime);
			pTimer->async_wait([pTimer, broadcast, endPoint, interval](const boost::system::error_code& ec) {
				if (ec) {
					if (ec == boost::asio::error::operation_aborted) {
						return;
					}

					CATAPULT_THROW_EXCEPTION(boost::system::system_error(ec));
				}

				broadcast();

				ScheduleBroadcast(pTimer, endPoint, interval, broadcast);
			});
		}
	}

	action CreateDefaultWaitForProposalPhaseEndAction(
			std::weak_ptr<WeightedVotingFsm> pFsmWeak,
			const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder,
			const extensions::PacketPayloadSink& packetPayloadSink) {
		return [pFsmWeak, pConfigHolder, packetPayloadSink]() {
			TRY_GET_FSM()

			auto& committeeData = pFsmShared->committeeData();
			const auto& stage = committeeData.committeeStage();
			auto phaseEndTimeMillis = GetPhaseEndTimeMillis(stage);

			DelayAction(
				pFsmShared,
				pFsmShared->timer(),
				phaseEndTimeMillis,
				[pFsmWeak] {
					TRY_GET_FSM()

					pFsmShared->processEvent(ProposalPhaseEnded{});
				}
			);

			auto pTimer = std::make_shared<boost::asio::system_timer>(pFsmShared->ioContext());
			auto interval = std::chrono::milliseconds(pConfigHolder->Config().Network.CommitteeMessageBroadcastInterval.millis());
			auto endPoint = stage.RoundStart + std::chrono::milliseconds(phaseEndTimeMillis) - 2 * interval;
			ScheduleBroadcast(
				pTimer,
				endPoint,
				interval,
				[pFsmWeak, packetPayloadSink]() {
					auto pFsmShared = pFsmWeak.lock();
					if (!pFsmShared)
						return;

					auto pBlock = pFsmShared->committeeData().proposedBlock();
					if (pBlock)
						packetPayloadSink(ionet::PacketPayloadFactory::FromEntity(ionet::PacketType::Push_Proposed_Block, pBlock));
				}
			);
		};
	}

	namespace {
		template<typename TPacket>
		bool IsSumOfVotesSufficient(
				const VoteMap<TPacket>& votes,
				double committeeApproval,
				double totalSumOfVotes,
				const chain::CommitteeManager& committeeManager) {
			double sum = 0.0;
			for (const auto& pair : votes)
				sum += committeeManager.weight(pair.first);

			return sum >= committeeApproval * totalSumOfVotes;
		}

		template<typename TPacket>
		action CreateWaitForVotesAction(
				std::weak_ptr<WeightedVotingFsm> pFsmWeak,
				const plugins::PluginManager& pluginManager,
				std::function<VoteMap<TPacket> (const CommitteeData&)> voteSupplier,
				consumer<bool> onVoteResult,
				const extensions::PacketPayloadSink& packetPayloadSink) {
			return [pFsmWeak, &pluginManager, voteSupplier, onVoteResult, packetPayloadSink]() {
				TRY_GET_FSM()

				const auto& config = pluginManager.configHolder()->Config().Network;
				auto& committeeData = pFsmShared->committeeData();
				const auto& stage = committeeData.committeeStage();
				auto delay = GetPhaseEndTimeMillis(stage);
				const auto& committeeManager = pluginManager.getCommitteeManager();

				DelayAction(
					pFsmShared,
					pFsmShared->timer(),
					delay,
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

				auto pTimer = std::make_shared<boost::asio::system_timer>(pFsmShared->ioContext());
				auto interval = std::chrono::milliseconds(config.CommitteeMessageBroadcastInterval.millis());
				auto endPoint = stage.RoundStart + std::chrono::milliseconds(delay) - 2 * interval;
				ScheduleBroadcast(
					pTimer,
					endPoint,
					interval,
					[pFsmWeak, packetPayloadSink, voteSupplier]() {
						auto pFsmShared = pFsmWeak.lock();
						if (!pFsmShared)
							return;

						auto votes = voteSupplier(pFsmShared->committeeData());
						for (const auto& pair : votes)
							packetPayloadSink(ionet::PacketPayload(pair.second));
					}
				);
			};
		}
	}

	action CreateDefaultWaitForPrevotesAction(
			std::weak_ptr<WeightedVotingFsm> pFsmWeak,
			const plugins::PluginManager& pluginManager,
			const extensions::PacketPayloadSink& packetPayloadSink) {
		return CreateWaitForVotesAction<PrevoteMessagePacket>(
			pFsmWeak,
			pluginManager,
			[](auto& committeeData) {
				return committeeData.prevotes();
			},
			[pFsmWeak] (bool sumOfPrevotesSufficient) {
				TRY_GET_FSM()

				pFsmShared->committeeData().setSumOfPrevotesSufficient(sumOfPrevotesSufficient);
				if (sumOfPrevotesSufficient) {
					pFsmShared->processEvent(SumOfPrevotesSufficient{});
				} else {
					pFsmShared->processEvent(SumOfPrevotesInsufficient{});
				}
			},
			packetPayloadSink);
	}

	action CreateDefaultWaitForPrecommitsAction(
			std::weak_ptr<WeightedVotingFsm> pFsmWeak,
			const plugins::PluginManager& pluginManager,
			const extensions::PacketPayloadSink& packetPayloadSink) {
		return CreateWaitForVotesAction<PrecommitMessagePacket>(
			pFsmWeak,
			pluginManager,
			[](auto& committeeData) {
				return committeeData.precommits();
			},
			[pFsmWeak] (bool sumOfPrecommitsSufficient) {
				TRY_GET_FSM()

				if (sumOfPrecommitsSufficient && pFsmShared->committeeData().sumOfPrevotesSufficient()) {
					pFsmShared->processEvent(SumOfPrecommitsSufficient{});
				} else {
					pFsmShared->processEvent(SumOfPrecommitsInsufficient{});
				}
			},
			packetPayloadSink);
	}

	namespace {
		template<typename TPacket>
		action CreateAddVoteAction(
				std::weak_ptr<WeightedVotingFsm> pFsmWeak,
				CommitteePhase phase) {
			return [pFsmWeak, phase]() {
				TRY_GET_FSM()

				SetPhase(pFsmShared, phase);

				auto& committeeData = pFsmShared->committeeData();

				auto pProposedBlock = committeeData.proposedBlock();
				if (!pProposedBlock)
					CATAPULT_THROW_RUNTIME_ERROR_1("add vote failed, no proposed block", phase);

				for (const auto* pKeyPair : committeeData.localCommittee()) {
					auto pPacket = ionet::CreateSharedPacket<TPacket>();
					pPacket->Message.Type = TPacket::Message_Type;
					pPacket->Message.BlockHash = committeeData.proposedBlockHash();
					auto& cosignature = pPacket->Message.BlockCosignature;
					cosignature.Signer = pKeyPair->publicKey();
					model::CosignBlockHeader(*pKeyPair, *pProposedBlock, cosignature.Signature);
					crypto::Sign(*pKeyPair, CommitteeMessageDataBuffer(*pPacket), pPacket->MessageSignature);
					committeeData.addVote(std::move(pPacket));
				}
			};
		}
	}

	action CreateDefaultAddPrevoteAction(std::weak_ptr<WeightedVotingFsm> pFsmWeak) {
		return CreateAddVoteAction<PrevoteMessagePacket>(pFsmWeak, CommitteePhase::Prevote);
	}

	action CreateDefaultAddPrecommitAction(std::weak_ptr<WeightedVotingFsm> pFsmWeak) {
		return CreateAddVoteAction<PrecommitMessagePacket>(pFsmWeak, CommitteePhase::Precommit);
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
					cosignatures.push_back(pair.second->Message.BlockCosignature);
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
			committeeData.setConfirmedBlock(pBlock);
		};
	}

	action CreateDefaultCommitConfirmedBlockAction(
			std::weak_ptr<WeightedVotingFsm> pFsmWeak,
			consumer<model::BlockRange&&, const disruptor::ProcessingCompleteFunc&> rangeConsumer,
			const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder,
			const extensions::PacketPayloadSink& packetPayloadSink,
			chain::CommitteeManager& committeeManager) {
		return [pFsmWeak, rangeConsumer, pConfigHolder, packetPayloadSink, &committeeManager]() {
			TRY_GET_FSM()

			SetPhase(pFsmShared, CommitteePhase::Commit);

			const auto& committeeData = pFsmShared->committeeData();
			auto pBlock = committeeData.confirmedBlock();
			if (!pBlock)
				CATAPULT_THROW_RUNTIME_ERROR("commit confirmed block failed, no block");

			const auto& stage = committeeData.committeeStage();
			if (stage.Round != pBlock->Round ||
				!ValidateBlockCosignatures(pBlock, committeeManager, pConfigHolder->Config().Network.CommitteeApproval)) {
				pFsmShared->processEvent(CommitBlockFailed{});
				return;
			}

			// Commit block.
			auto pPromise = std::make_shared<thread::promise<bool>>();
			rangeConsumer(model::BlockRange::FromEntity(pBlock), [pPromise](auto, const auto& result) {
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

			bool success = pPromise->get_future().get();

			if (success) {
				auto roundEndTimeMillis = GetPhaseEndTimeMillis(stage);
				DelayAction(pFsmShared, pFsmShared->timer(), roundEndTimeMillis, [pFsmWeak] {
					TRY_GET_FSM()

					pFsmShared->processEvent(CommitBlockSucceeded{});
				});

				auto pTimer = std::make_shared<boost::asio::system_timer>(pFsmShared->ioContext());
				auto interval = std::chrono::milliseconds(pConfigHolder->Config().Network.CommitteeMessageBroadcastInterval.millis());
				auto endPoint = stage.RoundStart + std::chrono::milliseconds(roundEndTimeMillis) - 2 * interval;
				ScheduleBroadcast(
					pTimer,
					endPoint,
					interval,
					[pFsmWeak, packetPayloadSink]() {
						auto pFsmShared = pFsmWeak.lock();
						if (!pFsmShared)
							return;

						auto pBlock = pFsmShared->committeeData().confirmedBlock();
						if (pBlock)
							packetPayloadSink(ionet::PacketPayloadFactory::FromEntity(ionet::PacketType::Push_Confirmed_Block, pBlock));
					}
				);
			} else {
				pFsmShared->processEvent(CommitBlockFailed{});
			}
		};
	}

	action CreateDefaultIncrementRoundAction(
			std::weak_ptr<WeightedVotingFsm> pFsmWeak,
			const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder) {
		return [pFsmWeak, pConfigHolder]() {
			TRY_GET_FSM()

			auto currentStage = pFsmShared->committeeData().committeeStage();
			pFsmShared->resetCommitteeData();

			int16_t nextRound = currentStage.Round + 1;
			auto nextRoundStart = currentStage.RoundStart + std::chrono::milliseconds(GetPhaseEndTimeMillis(currentStage));
			uint64_t nextPhaseTimeMillis = currentStage.PhaseTimeMillis;
			IncreasePhaseTime(nextPhaseTimeMillis, pConfigHolder->Config().Network);
			pFsmShared->committeeData().setCommitteeStage(CommitteeStage{
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

			auto currentStage = pFsmShared->committeeData().committeeStage();
			pFsmShared->resetCommitteeData();
			committeeManager.reset();

			const auto& config = pConfigHolder->Config().Network;
			auto nextRoundStart = currentStage.RoundStart + std::chrono::milliseconds(GetPhaseEndTimeMillis(currentStage));
			uint64_t nextPhaseTimeMillis = currentStage.PhaseTimeMillis;
			DecreasePhaseTime(nextPhaseTimeMillis, pConfigHolder->Config().Network);
			pFsmShared->committeeData().setCommitteeStage(CommitteeStage{
				0u,
				CommitteePhase::Propose,
				nextRoundStart,
				nextPhaseTimeMillis
			});
		};
	}

	action CreateDefaultWaitForConfirmedBlockAction(
		std::weak_ptr<WeightedVotingFsm> pFsmWeak,
		const model::BlockElementSupplier& lastBlockElementSupplier) {
		return [pFsmWeak, lastBlockElementSupplier]() {
			TRY_GET_FSM()

			auto& committeeData = pFsmShared->committeeData();
			committeeData.setProposedBlock(nullptr);
			committeeData.setConfirmedBlock(nullptr);
			SetPhase(pFsmShared, CommitteePhase::Commit);
			DelayAction(
				pFsmShared,
				pFsmShared->confirmedBlockWaitTimer(),
				GetPhaseEndTimeMillis(committeeData.committeeStage()),
				[pFsmWeak] {
					TRY_GET_FSM()

					pFsmShared->processEvent(ConfirmedBlockNotReceived{});
				},
				[pFsmWeak, lastBlockElementSupplier] {
					TRY_GET_FSM()

					auto pConfirmedBlock = pFsmShared->committeeData().confirmedBlock();
					if (!pConfirmedBlock)
						CATAPULT_THROW_RUNTIME_ERROR("wait for confirmed block failed, no block");

					if (pConfirmedBlock->Height > lastBlockElementSupplier()->Block.Height + Height(1)) {
						pFsmShared->processEvent(UnexpectedBlockHeight{});
					} else {
						pFsmShared->processEvent(ConfirmedBlockReceived{});
					}
				}
			);
		};
	}
}}