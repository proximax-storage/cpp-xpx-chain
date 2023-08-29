/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "WeightedVotingFsm.h"
#include "api/RemoteNodeApi.h"
#include "utils/WeightedVotingUtils.h"
#include "catapult/api/RemoteChainApi.h"
#include "catapult/chain/BlockDifficultyScorer.h"
#include "catapult/chain/ChainUtils.h"
#include "catapult/crypto/Signer.h"
#include "catapult/extensions/LocalNodeChainScore.h"
#include "catapult/extensions/ExecutionConfigurationFactory.h"
#include "catapult/extensions/PluginUtils.h"
#include "catapult/harvesting_core/UnlockedAccounts.h"
#include "catapult/model/BlockUtils.h"
#include "catapult/utils/StackLogger.h"
#include "catapult/validators/AggregateEntityValidator.h"

namespace catapult { namespace fastfinality {

	namespace {
		bool ApprovalRatingSufficient(
				const double approvalRating,
				const double totalRating,
				const model::NetworkConfiguration& config) {
			return approvalRating / totalRating >= config.CommitteeEndSyncApproval;
		}

		void DelayAction(
				const std::weak_ptr<WeightedVotingFsm>& pFsmWeak,
				boost::asio::system_timer& timer,
				utils::TimePoint expirationTime,
				const action& callback,
				const action& cancelledCallback = [](){}) {
			TRY_GET_FSM()

			auto& committeeData = pFsmShared->committeeData();
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
				const std::weak_ptr<WeightedVotingFsm>& pFsmWeak,
				boost::asio::system_timer& timer,
				uint64_t delay,
				const action& callback,
				const action& cancelledCallback = [](){}) {
			TRY_GET_FSM()

			auto expirationTime = pFsmShared->committeeData().committeeRound().RoundStart + std::chrono::milliseconds(delay);
			DelayAction(pFsmWeak, timer, expirationTime, callback, cancelledCallback);
		}
	}

	action CreateDefaultCheckLocalChainAction(
			const std::weak_ptr<WeightedVotingFsm>& pFsmWeak,
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
				DelayAction(pFsmWeak, pFsmShared->timer(), config.CommitteeChainHeightRequestInterval.millis(), [pFsmWeak] {
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
					DelayAction(pFsmWeak, pFsmShared->timer(), config.CommitteeChainHeightRequestInterval.millis(), [pFsmWeak] {
						TRY_GET_FSM()

						pFsmShared->processEvent(NetworkHeightDetectionFailure{});
					});
				}
			}
		};
	}

	action CreateDefaultResetLocalChainAction() {
		return []() {
			CATAPULT_THROW_RUNTIME_ERROR("local chain is invalid and needs to be reset")
		};
	}

	namespace {
		constexpr auto UNITS_IN_THE_LAST_PLACE = 2;

		bool IsSumOfVotesSufficient(double actual, double min) {
			return actual >= min
				|| std::abs(actual - min) <= std::numeric_limits<double>::epsilon() * std::abs(actual + min) * UNITS_IN_THE_LAST_PLACE
				|| std::abs(actual - min) <= std::numeric_limits<double>::min();
		}

		bool ValidateBlockCosignatures(
				const std::shared_ptr<model::Block>& pBlock,
				const chain::CommitteeManager& committeeManager,
				double committeeApproval,
				double totalSumOfVotes = 0.0) {
			const auto& committee = committeeManager.committee();
			if (pBlock->Signer != committee.BlockProposer) {
				CATAPULT_LOG(warning) << "rejecting block, signer " << pBlock->Signer
					<< " invalid, expected " << committee.BlockProposer;
				return false;
			}

			if (!model::VerifyBlockHeaderSignature(*pBlock)) {
				CATAPULT_LOG(warning) << "rejecting block, signature invalid";
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

				if (!model::VerifyBlockHeaderCosignature(*pBlock, *pCosignature)) {
					CATAPULT_LOG(warning) << "rejecting block, cosignature invalid";
					return false;
				}

				actualSumOfVotes += committeeManager.weight(pCosignature->Signer);
			}

			if (totalSumOfVotes == 0.0) {
				totalSumOfVotes = blockProposerWeight;
				for (const auto& cosigner : committee.Cosigners)
					totalSumOfVotes += committeeManager.weight(cosigner);
			}

			auto minSumOfVotes = committeeApproval * totalSumOfVotes;
			if (!IsSumOfVotesSufficient(actualSumOfVotes, minSumOfVotes)) {
				CATAPULT_LOG(warning) << "rejecting block, sum of votes insufficient: " << std::fixed << std::setprecision(30) << actualSumOfVotes << " < " << minSumOfVotes;
				return false;
			}

			return true;
		}
	}

	action CreateDefaultDownloadBlocksAction(
			const std::weak_ptr<WeightedVotingFsm>& pFsmWeak,
			extensions::ServiceState& state,
			const consumer<model::BlockRange&&, const disruptor::ProcessingCompleteFunc&>& rangeConsumer,
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
					while (committeeManager.committee().Round < pBlock->round())
						committeeManager.selectCommittee(config.Network);
					CATAPULT_LOG(debug) << "selected committee for round " << pBlock->round();

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

	action CreateDefaultDetectStageAction(
			const std::weak_ptr<WeightedVotingFsm>& pFsmWeak,
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

			auto roundStart = block.Timestamp + Timestamp(chain::CommitteePhaseCount * block.committeePhaseTime());
			auto currentTime = timeSupplier();
			if (block.Timestamp > currentTime)
				CATAPULT_THROW_RUNTIME_ERROR_2("invalid current time", currentTime, block.Timestamp)

			auto phaseTimeMillis = block.committeePhaseTime() ? block.committeePhaseTime() : config.CommitteePhaseTime.millis();
			chain::DecreasePhaseTime(phaseTimeMillis, config);
			auto nextRoundStart = roundStart + Timestamp(chain::CommitteePhaseCount * phaseTimeMillis);
			committeeManager.selectCommittee(config);

			while (nextRoundStart < timeSupplier()) {
				roundStart = nextRoundStart;
				chain::IncreasePhaseTime(phaseTimeMillis, config);
				nextRoundStart = nextRoundStart + Timestamp(chain::CommitteePhaseCount * phaseTimeMillis);

				committeeManager.selectCommittee(config);
			}

			CommitteeRound round {
				committeeManager.committee().Round,
				(timeSupplier() < roundStart +
					Timestamp(phaseTimeMillis - config.CommitteeRequestInterval.millis() - config.CommitteeSilenceInterval.millis())) ?
						CommitteePhase::Propose : CommitteePhase::Prevote,
				utils::ToTimePoint(roundStart),
				phaseTimeMillis
			};

			CATAPULT_LOG(debug) << "start phase " << round.StartPhase << ", phase time " << phaseTimeMillis << "ms, round " << round.Round;
			pFsmShared->committeeData().setCommitteeRound(round);
			pFsmShared->processEvent(StageDetectionSucceeded{});
		};
	}

	action CreateDefaultSelectCommitteeAction(
			const std::weak_ptr<WeightedVotingFsm>& pFsmWeak,
			chain::CommitteeManager& committeeManager,
			const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder) {
		return [pFsmWeak, &committeeManager, pConfigHolder]() {
			TRY_GET_FSM()

			pFsmShared->dbrbProcess()->updateView(pConfigHolder);

			auto& committeeData = pFsmShared->committeeData();
			auto round = committeeData.committeeRound();
			if (CommitteePhase::None == round.StartPhase)
				CATAPULT_THROW_RUNTIME_ERROR("committee start phase is not set")

			if (committeeManager.committee().Round > round.Round)
				CATAPULT_THROW_RUNTIME_ERROR_2("invalid committee round", committeeManager.committee().Round, round.Round)

			const auto& config = pConfigHolder->Config().Network;
			while (committeeManager.committee().Round < round.Round)
				committeeManager.selectCommittee(config);
			CATAPULT_LOG(debug) << "selected committee for round " << round.Round;

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

			bool isCosigner = false;
			std::for_each(accounts.begin(), accounts.end(), [&cosigners, &localCommittee, &isCosigner](const auto& keyPair) {
				auto cosignerIter = std::find_if(cosigners.begin(), cosigners.end(), [&keyPair](const auto& cosigner) {
					return (cosigner == keyPair.publicKey());
				});
				if (cosignerIter != cosigners.end()) {
					localCommittee.insert(&keyPair);
					isCosigner = true;
				}
			});

			double totalSumOfVotes = committeeManager.weight(committee.BlockProposer);
			for (const auto& cosigner : committee.Cosigners)
				totalSumOfVotes += committeeManager.weight(cosigner);
			committeeData.setTotalSumOfVotes(totalSumOfVotes);

			CATAPULT_LOG(debug) << "committee selection result: is block proposer = " << isBlockProposer << ", is cosigner = " << isCosigner << ", start phase = " << round.StartPhase << ", phase time = " << round.PhaseTimeMillis << "ms";

			pFsmShared->processEvent(CommitteeSelectionResult{ isBlockProposer, round.StartPhase });
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
			{}

		public:
			const model::Block& ParentBlock;
			model::PreviousBlockContext ParentContext;
			catapult::Timestamp Timestamp;
			catapult::Height Height;
			catapult::Difficulty Difficulty;

		public:
			bool tryCalculateDifficulty(const cache::BlockDifficultyCache& cache, const model::NetworkConfiguration& config) {
				return chain::TryCalculateDifficulty(cache, state::BlockDifficultyInfo(Height, Timestamp, Difficulty), config, Difficulty);
			}
		};
	}

	namespace {
		auto GetPhaseEndTimeMillis(CommitteePhase phase, uint64_t phaseTimeMillis) {
			switch (phase) {
			case CommitteePhase::Propose:
				return phaseTimeMillis;
			case CommitteePhase::Prevote:
				return 2 * phaseTimeMillis;
			case CommitteePhase::Precommit:
				return 3 * phaseTimeMillis;
			case CommitteePhase::Commit:
				return 4 * phaseTimeMillis;
			default:
				CATAPULT_THROW_RUNTIME_ERROR_1("invalid committee phase", phase)
			}
		}
	}

	action CreateDefaultProposeBlockAction(
			const std::weak_ptr<WeightedVotingFsm>& pFsmWeak,
			const cache::CatapultCache& cache,
			const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder,
			const harvesting::BlockGenerator& blockGenerator,
			const model::BlockElementSupplier& lastBlockElementSupplier) {
		return [pFsmWeak, &cache, pConfigHolder, blockGenerator, lastBlockElementSupplier]() {
			TRY_GET_FSM()

			auto& committeeData = pFsmShared->committeeData();
			committeeData.setProposedBlock(nullptr);
			auto committeeRound = committeeData.committeeRound();
			auto pParentBlockElement = lastBlockElementSupplier();
			NextBlockContext context(*pParentBlockElement, utils::FromTimePoint(committeeRound.RoundStart));
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
			pBlockHeader->setRound(committeeRound.Round);
			pBlockHeader->setCommitteePhaseTime(committeeRound.PhaseTimeMillis);
			auto pBlock = utils::UniqueToShared(blockGenerator(*pBlockHeader, config.Network.MaxTransactionsPerBlock));
			if (pBlock) {
				model::SignBlockHeader(*committeeData.blockProposer(), *pBlock);
				committeeData.setProposedBlock(pBlock);
				auto pPacket = ionet::CreateSharedPacket<ionet::Packet>(pBlock->Size);
				pPacket->Type = ionet::PacketType::Push_Proposed_Block;
				std::memcpy(static_cast<void*>(pPacket->Data()), pBlock.get(), pBlock->Size);
				pFsmShared->dbrbProcess()->broadcast(pPacket);

				auto phaseEndTimeMillis = GetPhaseEndTimeMillis(CommitteePhase::Propose, committeeRound.PhaseTimeMillis);
				DelayAction(pFsmWeak, pFsmShared->timer(), phaseEndTimeMillis, [pFsmWeak] {
					TRY_GET_FSM()

					pFsmShared->processEvent(BlockProposingSucceeded{});
				});
			} else {
				pFsmShared->processEvent(BlockProposingFailed{});
			}
		};
	}

	namespace {
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

		bool ValidateProposedBlock(
				const std::shared_ptr<model::Block>& pProposedBlock,
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
			const std::weak_ptr<WeightedVotingFsm>& pFsmWeak,
			extensions::ServiceState& state,
			const model::BlockElementSupplier& lastBlockElementSupplier,
			const std::shared_ptr<thread::IoThreadPool>& pValidatorPool) {
		return [pFsmWeak, &state, lastBlockElementSupplier, pValidatorPool]() {
			TRY_GET_FSM()

			auto& committeeData = pFsmShared->committeeData();
			auto pProposedBlock = committeeData.proposedBlock();
			if (!pProposedBlock)
				CATAPULT_THROW_RUNTIME_ERROR("no proposal to validate")

			if (pProposedBlock->Height != lastBlockElementSupplier()->Block.Height + Height(1)) {
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

			bool isProposedBlockValid = false;
			try {
				isProposedBlockValid = ValidateProposedBlock(pProposedBlock, state, lastBlockElementSupplier, pValidatorPool);
			} catch (...) {} // empty catch block is intentional

			if (isProposedBlockValid) {
				auto phaseEndTimeMillis = GetPhaseEndTimeMillis(CommitteePhase::Propose, committeeData.committeeRound().PhaseTimeMillis);
				DelayAction(pFsmWeak, pFsmShared->timer(), phaseEndTimeMillis, [pFsmWeak] {
					TRY_GET_FSM()

					pFsmShared->processEvent(ProposalValid{});
				});
			} else {
				committeeData.setProposedBlock(nullptr);
				pFsmShared->processEvent(ProposalInvalid{});
			}
		};
	}

	action CreateDefaultWaitForProposalAction(
			const std::weak_ptr<WeightedVotingFsm>& pFsmWeak) {
		return [pFsmWeak]() {
			TRY_GET_FSM()

			auto& committeeData = pFsmShared->committeeData();
			if (committeeData.proposedBlock()) {
				pFsmShared->processEvent(ProposalReceived{});
			} else {
				auto future = committeeData.startWaitForProposedBlock();
				auto phaseEndTimeMillis = GetPhaseEndTimeMillis(CommitteePhase::Propose, committeeData.committeeRound().PhaseTimeMillis);
				auto timeout = committeeData.committeeRound().RoundStart + std::chrono::milliseconds(phaseEndTimeMillis);
				try {
					auto status = future.wait_until(timeout);
					if (std::future_status::ready == status) {
						pFsmShared->processEvent(ProposalReceived{});
					}else {
						pFsmShared->processEvent(ProposalNotReceived{});
					}
				} catch(...) {}
			}
		};
	}

	namespace {
		bool IsSumOfVotesSufficient(
				const VoteMap& votes,
				double committeeApproval,
				double totalSumOfVotes,
				const chain::CommitteeManager& committeeManager) {
			double actualSumOfVotes = 0.0;
			for (const auto& pair : votes)
				actualSumOfVotes += committeeManager.weight(pair.first);

			auto minSumOfVotes = committeeApproval * totalSumOfVotes;
			auto sumOfVotesSufficient = IsSumOfVotesSufficient(actualSumOfVotes, minSumOfVotes);
			if (!sumOfVotesSufficient)
				CATAPULT_LOG(debug) << "sum of votes insufficient: " << std::fixed << std::setprecision(30) << actualSumOfVotes << " < " << minSumOfVotes << ", vote count " << votes.size();
			return sumOfVotesSufficient;
		}
	}

	action CreateDefaultWaitForPrevotePhaseEndAction(
			const std::weak_ptr<WeightedVotingFsm>& pFsmWeak,
			chain::CommitteeManager& committeeManager,
			const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder) {
		return [pFsmWeak, &committeeManager, pConfigHolder]() {
			TRY_GET_FSM()

			auto phaseEndTimeMillis = GetPhaseEndTimeMillis(CommitteePhase::Prevote, pFsmShared->committeeData().committeeRound().PhaseTimeMillis);
			DelayAction(pFsmWeak, pFsmShared->timer(), phaseEndTimeMillis, [pFsmWeak, &committeeManager, pConfigHolder] {
				TRY_GET_FSM()

				const auto& config = pConfigHolder->Config().Network;
				auto& committeeData = pFsmShared->committeeData();
				bool sumOfPrevotesSufficient = IsSumOfVotesSufficient(
						committeeData.prevotes(),
						config.CommitteeApproval,
						committeeData.totalSumOfVotes(),
						committeeManager);

				committeeData.setSumOfPrevotesSufficient(sumOfPrevotesSufficient);
				if (sumOfPrevotesSufficient) {
					pFsmShared->processEvent(SumOfPrevotesSufficient{});
				} else {
					pFsmShared->processEvent(SumOfPrevotesInsufficient{});
				}
			});
		};
	}

	namespace {
		template<typename TPacket, CommitteeMessageType MessageType, CommitteePhase Phase>
		action CreateAddVoteAction(
				const std::weak_ptr<WeightedVotingFsm>& pFsmWeak) {
			return [pFsmWeak]() {
				TRY_GET_FSM()

				auto& committeeData = pFsmShared->committeeData();
				auto pProposedBlock = committeeData.proposedBlock();
				if (!pProposedBlock)
					CATAPULT_THROW_RUNTIME_ERROR_1("add vote failed, no proposed block", Phase)

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
				if (votes.empty())
					return;

				auto pPacket = ionet::CreateSharedPacket<TPacket>(utils::checked_cast<size_t, uint32_t>(sizeof(CommitteeMessage) * votes.size()));
				pPacket->MessageCount = utils::checked_cast<size_t, uint8_t>(votes.size());

				auto* pMessage = reinterpret_cast<CommitteeMessage*>(pPacket.get() + 1);
				auto index = 0u;
				for (const auto& pair : votes)
					pMessage[index++] = pair.second;

				pFsmShared->dbrbProcess()->broadcast(pPacket);
			};
		}
	}

	action CreateDefaultAddPrevoteAction(const std::weak_ptr<WeightedVotingFsm>& pFsmWeak) {
		return CreateAddVoteAction<PushPrevoteMessagesRequest, CommitteeMessageType::Prevote, CommitteePhase::Prevote>(pFsmWeak);
	}

	action CreateDefaultAddPrecommitAction(const std::weak_ptr<WeightedVotingFsm>& pFsmWeak) {
		return CreateAddVoteAction<PushPrecommitMessagesRequest, CommitteeMessageType::Precommit, CommitteePhase::Precommit>(pFsmWeak);
	}

	action CreateDefaultWaitForPrecommitPhaseEndAction(
			const std::weak_ptr<WeightedVotingFsm>& pFsmWeak,
			chain::CommitteeManager& committeeManager,
			const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder) {
		return [pFsmWeak, &committeeManager, pConfigHolder]() {
			TRY_GET_FSM()

			auto phaseEndTimeMillis = GetPhaseEndTimeMillis(CommitteePhase::Precommit, pFsmShared->committeeData().committeeRound().PhaseTimeMillis);
			DelayAction(pFsmWeak, pFsmShared->timer(), phaseEndTimeMillis, [pFsmWeak, &committeeManager, pConfigHolder] {
				TRY_GET_FSM()

				const auto& config = pConfigHolder->Config().Network;
				const auto& committeeData = pFsmShared->committeeData();
				bool sumOfPrecommitsSufficient = IsSumOfVotesSufficient(
						committeeData.precommits(),
						config.CommitteeApproval,
						committeeData.totalSumOfVotes(),
						committeeManager);

				if (sumOfPrecommitsSufficient && committeeData.sumOfPrevotesSufficient()) {
					pFsmShared->processEvent(SumOfPrecommitsSufficient{});
				} else {
					pFsmShared->processEvent(SumOfPrecommitsInsufficient{});
				}
			});
		};
	}

	action CreateDefaultUpdateConfirmedBlockAction(
		const std::weak_ptr<WeightedVotingFsm>& pFsmWeak,
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
				CATAPULT_THROW_RUNTIME_ERROR("update confirmed block failed, no proposed block")

			auto blockSize = pProposedBlock->Size + cosignaturesSize;
			auto pBlock = utils::MakeSharedWithSize<model::Block>(blockSize);
			std::memcpy(static_cast<void *>(pBlock.get()), pProposedBlock.get(), pProposedBlock->Size);
			pBlock->Size = blockSize;
			std::memcpy(static_cast<void *>(pBlock->CosignaturesPtr()), cosignatures.data(), cosignaturesSize);
			committeeData.setProposedBlock(nullptr);
			committeeData.setConfirmedBlock(pBlock);
		};
	}

	action CreateDefaultRequestConfirmedBlockAction(
		const std::weak_ptr<WeightedVotingFsm>& pFsmWeak,
		extensions::ServiceState& state,
		const model::BlockElementSupplier& lastBlockElementSupplier) {
		return [pFsmWeak, &state, lastBlockElementSupplier]() {
			TRY_GET_FSM()

			auto phaseTimeMillis = pFsmShared->committeeData().committeeRound().PhaseTimeMillis;
			auto requestInterval = state.config().Network.CommitteeRequestInterval.millis();
			auto timeout = GetPhaseEndTimeMillis(CommitteePhase::Precommit, phaseTimeMillis) + requestInterval;
			DelayAction(pFsmWeak, pFsmShared->timer(), timeout, [pFsmWeak, &state, lastBlockElementSupplier, phaseTimeMillis] {
				TRY_GET_FSM()

				auto timeout = utils::TimeSpan::FromSeconds(60);
				auto packetIoPairs = state.packetIoPickers().pickMultiple(timeout);
				CATAPULT_LOG(debug) << "found " << packetIoPairs.size() << " peer(s) for pulling confirmed block";
				auto expectedHeight = lastBlockElementSupplier()->Block.Height + Height(1);
				const auto& getCommitteeManager = state.pluginManager().getCommitteeManager();
				auto& committeeData = pFsmShared->committeeData();
				for (const auto& packetIoPair : packetIoPairs) {
					auto pRemoteApi = CreateRemoteNodeApi(*packetIoPair.io(), state.pluginManager().transactionRegistry());
					std::shared_ptr<model::Block> pBlock;

					try {
						pBlock = pRemoteApi->confirmedBlock().get();
					} catch (...) {}

					if (pBlock) {
						if (pBlock->Height != expectedHeight) {
							CATAPULT_LOG(debug) << "got block at unexpected height " << pBlock->Height << ", expected at " << expectedHeight;
							pFsmShared->processEvent(UnexpectedBlockHeight{});
							return;
						}

						const auto& committee = getCommitteeManager.committee();
						if (pBlock->round() != committee.Round) {
							CATAPULT_LOG(warning) << "rejecting block, round " << pBlock->round() << " invalid, expected " << committee.Round;
							continue;
						}

						if (!ValidateBlockCosignatures(pBlock, getCommitteeManager, state.config().Network.CommitteeApproval, committeeData.totalSumOfVotes()))
							continue;

						committeeData.setConfirmedBlock(pBlock);
						pFsmShared->processEvent(ConfirmedBlockReceived{});
						return;
					}
				}

				DelayAction(pFsmWeak, pFsmShared->timer(), GetPhaseEndTimeMillis(CommitteePhase::Commit, phaseTimeMillis), [pFsmWeak, &state] {
					TRY_GET_FSM()

					pFsmShared->processEvent(ConfirmedBlockNotReceived{});
				});
			});
		};
	}

	action CreateDefaultCommitConfirmedBlockAction(
			const std::weak_ptr<WeightedVotingFsm>& pFsmWeak,
			const consumer<model::BlockRange&&, const disruptor::ProcessingCompleteFunc&>& rangeConsumer,
			extensions::ServiceState& state) {
		return [pFsmWeak, rangeConsumer, &state]() {
			TRY_GET_FSM()

			auto pConfigHolder = state.pluginManager().configHolder();
			auto& committeeManager = state.pluginManager().getCommitteeManager();

			bool success = false;
			auto& committeeData = pFsmShared->committeeData();
			{
				// Commit block.
				std::lock_guard<std::mutex> guard(pFsmShared->mutex());

				auto pPromise = std::make_shared<thread::promise<bool>>();
				rangeConsumer(model::BlockRange::FromEntity(committeeData.confirmedBlock()), [pPromise](auto, const auto& result) {
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

			DelayAction(pFsmWeak, pFsmShared->timer(), GetPhaseEndTimeMillis(CommitteePhase::Commit, committeeData.committeeRound().PhaseTimeMillis), [pFsmWeak, success, &state] {
				TRY_GET_FSM()

				auto maxChainHeight = state.maxChainHeight();
				if (success && (maxChainHeight > Height(0)) && (pFsmShared->committeeData().confirmedBlock()->Height >= maxChainHeight)) {
					pFsmShared->processEvent(Hold{});
				} else if (success) {
					pFsmShared->processEvent(CommitBlockSucceeded{});
				} else {
					pFsmShared->processEvent(CommitBlockFailed{});
				}
			});
		};
	}

	action CreateDefaultIncrementRoundAction(
			const std::weak_ptr<WeightedVotingFsm>& pFsmWeak,
			const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder) {
		return [pFsmWeak, pConfigHolder]() {
			TRY_GET_FSM()

			auto& committeeData = pFsmShared->committeeData();
			auto currentRound = committeeData.committeeRound();
			pFsmShared->resetCommitteeData();

			int64_t nextRound = currentRound.Round + 1;
			CATAPULT_LOG(debug) << "incremented round " << nextRound;
			auto nextRoundStart = currentRound.RoundStart + std::chrono::milliseconds(GetPhaseEndTimeMillis(CommitteePhase::Commit, currentRound.PhaseTimeMillis));
			uint64_t nextPhaseTimeMillis = currentRound.PhaseTimeMillis;
			chain::IncreasePhaseTime(nextPhaseTimeMillis, pConfigHolder->Config().Network);
			committeeData.setCommitteeRound(CommitteeRound{
				nextRound,
				CommitteePhase::Propose,
				nextRoundStart,
				nextPhaseTimeMillis
			});
		};
	}

	action CreateDefaultResetRoundAction(
			const std::weak_ptr<WeightedVotingFsm>& pFsmWeak,
			const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder,
			chain::CommitteeManager& committeeManager) {
		return [pFsmWeak, pConfigHolder, &committeeManager]() {
			TRY_GET_FSM()

			auto& committeeData = pFsmShared->committeeData();
			auto currentRound = committeeData.committeeRound();
			pFsmShared->resetCommitteeData();
			committeeManager.reset();

			const auto& config = pConfigHolder->Config().Network;
			auto nextRoundStart = currentRound.RoundStart + std::chrono::milliseconds(GetPhaseEndTimeMillis(CommitteePhase::Commit, currentRound.PhaseTimeMillis));
			uint64_t nextPhaseTimeMillis = currentRound.PhaseTimeMillis;
			chain::DecreasePhaseTime(nextPhaseTimeMillis, pConfigHolder->Config().Network);
			committeeData.setCommitteeRound(CommitteeRound{
				0u,
				CommitteePhase::Propose,
				nextRoundStart,
				nextPhaseTimeMillis
			});
		};
	}
}}