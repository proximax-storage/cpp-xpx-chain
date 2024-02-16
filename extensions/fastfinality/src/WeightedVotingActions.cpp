/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "WeightedVotingFsm.h"
#include "utils/WeightedVotingUtils.h"
#include "catapult/api/RemoteChainApi.h"
#include "catapult/chain/BlockDifficultyScorer.h"
#include "catapult/crypto/Signer.h"
#include "catapult/extensions/LocalNodeChainScore.h"
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
			const std::function<uint64_t (const Key&)>& importanceGetter) {
		return [pFsmWeak, retriever, pConfigHolder, lastBlockElementSupplier, importanceGetter]() {
			TRY_GET_FSM()

			pFsmShared->setNodeWorkState(NodeWorkState::Synchronizing);
			pFsmShared->resetChainSyncData();
			pFsmShared->resetCommitteeData();

			auto localHeight = lastBlockElementSupplier()->Block.Height;
			bool isInDbrbSystem = pFsmShared->dbrbProcess()->updateView(pConfigHolder, utils::NetworkTime(), localHeight, false);

			std::vector<RemoteNodeState> remoteNodeStates = retriever();

		  	const auto& config = pConfigHolder->Config().Network;
		  	if (remoteNodeStates.empty()) {
				DelayAction(pFsmWeak, pFsmShared->timer(), config.CommitteeChainHeightRequestInterval.millis(), [pFsmWeak] {
					TRY_GET_FSM()

					CATAPULT_LOG(debug) << "got no remote node states";
					pFsmShared->processEvent(NetworkHeightDetectionFailure{});
				});
				return;
			}

			std::sort(remoteNodeStates.begin(), remoteNodeStates.end(), [](auto a, auto b) {
				return (a.Height == b.Height ? a.BlockHash > b.BlockHash : a.Height > b.Height);
			});

			auto& chainSyncData = pFsmShared->chainSyncData();
			chainSyncData.NetworkHeight = remoteNodeStates.begin()->Height;
			chainSyncData.LocalHeight = localHeight;

			if (chainSyncData.NetworkHeight < chainSyncData.LocalHeight) {

				pFsmShared->processEvent(NetworkHeightLessThanLocal{});

			} else if (chainSyncData.NetworkHeight > chainSyncData.LocalHeight) {

				std::map<Hash256, std::pair<uint64_t, std::vector<Key>>> hashKeys;

				for (const auto& state : remoteNodeStates) {
					if (state.Height < chainSyncData.NetworkHeight)
						break;

					auto& pair = hashKeys[state.BlockHash];
					pair.first += importanceGetter(state.NodeKey);
					for (const auto& key : state.HarvesterKeys)
						pair.first += importanceGetter(key);
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
					for (const auto& key : state.HarvesterKeys)
						importance += importanceGetter(key);
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
					if (isInDbrbSystem) {
						pFsmShared->processEvent(NetworkHeightEqualToLocal{});
					} else {
						pFsmShared->dbrbProcess()->updateView(pConfigHolder, utils::NetworkTime(), localHeight, true);
						DelayAction(pFsmWeak, pFsmShared->timer(), config.CommitteeChainHeightRequestInterval.millis(), [pFsmWeak] {
							TRY_GET_FSM()

							CATAPULT_LOG(debug) << "not registered in the DBRB system";
							pFsmShared->processEvent(NotRegisteredInDbrbSystem{});
						});
					}
				} else {
					DelayAction(pFsmWeak, pFsmShared->timer(), config.CommitteeChainHeightRequestInterval.millis(), [pFsmWeak] {
						TRY_GET_FSM()

						CATAPULT_LOG(debug) << "approval rating not sufficient";
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
		bool ValidateBlockCosignatures(const std::shared_ptr<model::Block>& pBlock, const chain::CommitteeManager& committeeManager, const model::NetworkConfiguration& config) {
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

			auto blockProposerWeight = committeeManager.weight(committee.BlockProposer, config);
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

				committeeManager.add(actualSumOfVotes, committeeManager.weight(pCosignature->Signer, config));
			}

			chain::HarvesterWeight totalSumOfVotes = committeeManager.zeroWeight();
			for (const auto& cosigner : committee.Cosigners)
				committeeManager.add(totalSumOfVotes, committeeManager.weight(cosigner, config));

			auto minSumOfVotes = totalSumOfVotes;
			committeeManager.mul(minSumOfVotes, config.CommitteeApproval);
			if (!committeeManager.ge(actualSumOfVotes, minSumOfVotes)) {
				CATAPULT_LOG(warning) << "rejecting block, sum of votes insufficient: " << committeeManager.str(actualSumOfVotes) << " < " << committeeManager.str(minSumOfVotes);
				return false;
			}

			return true;
		}
	}

	action CreateDefaultDownloadBlocksAction(
			const std::weak_ptr<WeightedVotingFsm>& pFsmWeak,
			extensions::ServiceState& state,
			const consumer<model::BlockRange&&, const disruptor::ProcessingCompleteFunc&>& rangeConsumer) {
		return [pFsmWeak, &state, rangeConsumer]() {
			TRY_GET_FSM()

			const auto& nodeConfig = state.config().Node;
			auto maxBlocksPerSyncAttempt = nodeConfig.MaxBlocksPerSyncAttempt;
			const auto& chainSyncData = pFsmShared->chainSyncData();
			auto startHeight = chainSyncData.LocalHeight + Height(1);
			auto targetHeight = std::min(chainSyncData.NetworkHeight, chainSyncData.LocalHeight + Height(maxBlocksPerSyncAttempt));
			api::BlocksFromOptions blocksFromOptions{
				utils::checked_cast<uint64_t, uint32_t>(targetHeight.unwrap() - chainSyncData.LocalHeight.unwrap()),
				nodeConfig.MaxChainBytesPerSyncAttempt.bytes32()
			};

			const auto& pDbrbProcess = pFsmShared->dbrbProcess();
			auto pMessageSender = pDbrbProcess->messageSender();
			pMessageSender->clearQueue();
			for (const auto& identityKey : chainSyncData.NodeIdentityKeys) {
				std::vector<std::shared_ptr<model::Block>> blocks;
				{
					auto packetIoPair = pMessageSender->getNodePacketIoPair(identityKey);
					if (!packetIoPair) {
						CATAPULT_LOG(debug) << "no packet IO to get blocks from " << identityKey;
						continue;
					}

					auto pRemoteChainApi = api::CreateRemoteChainApi(
						*packetIoPair.io(),
						identityKey,
						state.pluginManager().transactionRegistry());

					try {
						auto blockRange = pRemoteChainApi->blocksFrom(startHeight, blocksFromOptions).get();
						blocks = model::EntityRange<model::Block>::ExtractEntitiesFromRange(std::move(blockRange));
						pMessageSender->pushNodePacketIoPair(identityKey, packetIoPair);
					} catch (std::exception const& error) {
						CATAPULT_LOG(warning) << "error downloading blocks: " << error.what();
						pMessageSender->removeNode(identityKey);
						continue;
					} catch (...) {
						CATAPULT_LOG(warning) << "error downloading blocks: unknown error";
						pMessageSender->removeNode(identityKey);
						continue;
					}
				}

				bool success = false;
				for (const auto& pBlock : blocks) {
					const auto& config = state.config(pBlock->Height).Network;
					auto& committeeManager = state.pluginManager().getCommitteeManager(pBlock->EntityVersion());
					committeeManager.reset();
					while (committeeManager.committee().Round < pBlock->round())
						committeeManager.selectCommittee(config);
					CATAPULT_LOG(debug) << "block " << pBlock->Height << ": selected committee for round " << pBlock->round();
					committeeManager.logCommittee();

					if (ValidateBlockCosignatures(pBlock, committeeManager, config)) {
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

			DelayAction(pFsmWeak, pFsmShared->timer(), state.config().Network.CommitteeChainHeightRequestInterval.millis(), [pFsmWeak] {
				TRY_GET_FSM()

				pFsmShared->processEvent(DownloadBlocksFailed{});
			});
		};
	}

	namespace {
		std::string GetTimeString(const utils::TimePoint& timestamp) {
			auto time = std::chrono::system_clock::to_time_t(timestamp);
			char buffer[40];
			std::strftime(buffer, 40 ,"%F %T", std::localtime(&time));
			return buffer;
		}
	}

	action CreateDefaultDetectStageAction(
			const std::weak_ptr<WeightedVotingFsm>& pFsmWeak,
			const chain::TimeSupplier& timeSupplier,
			const model::BlockElementSupplier& lastBlockElementSupplier,
			extensions::ServiceState& state) {
		return [pFsmWeak, timeSupplier, lastBlockElementSupplier, &state]() {
			TRY_GET_FSM()

		  	pFsmShared->resetChainSyncData();
			pFsmShared->setNodeWorkState(NodeWorkState::Running);
			auto& committeeManager = state.pluginManager().getCommitteeManager(model::Block::Current_Version);
			committeeManager.reset();

			auto pLastBlockElement = lastBlockElementSupplier();
			const auto& block = pLastBlockElement->Block;
			const auto& config = state.pluginManager().config(block.Height + Height(1));

			auto roundStart = block.Timestamp + Timestamp(chain::CommitteePhaseCount * block.committeePhaseTime());
			auto currentTime = timeSupplier();
			if (block.Timestamp > currentTime)
				CATAPULT_THROW_RUNTIME_ERROR_2("invalid current time", currentTime, block.Timestamp)

			auto phaseTimeMillis = block.committeePhaseTime() ? block.committeePhaseTime() : config.CommitteePhaseTime.millis();
			chain::DecreasePhaseTime(phaseTimeMillis, config);
			auto nextRoundStart = roundStart + Timestamp(chain::CommitteePhaseCount * phaseTimeMillis);
			committeeManager.selectCommittee(config);

			auto committeeSilenceInterval = Timestamp(config.CommitteeSilenceInterval.millis());
			while (nextRoundStart <= timeSupplier() + committeeSilenceInterval) {
				roundStart = nextRoundStart;
				chain::IncreasePhaseTime(phaseTimeMillis, config);
				nextRoundStart = nextRoundStart + Timestamp(chain::CommitteePhaseCount * phaseTimeMillis);

				committeeManager.selectCommittee(config);
			}

			CommitteePhase startPhase;
			if (timeSupplier() >= roundStart) {
				startPhase = static_cast<CommitteePhase>((timeSupplier() - roundStart).unwrap() / phaseTimeMillis + 1);
			} else {
				startPhase = CommitteePhase::Propose;
			}

			CommitteeRound round {
				committeeManager.committee().Round,
				startPhase,
				utils::ToTimePoint(roundStart),
				phaseTimeMillis
			};

			CATAPULT_LOG(debug) << "detected stage: start phase " << round.StartPhase << ", start time " << GetTimeString(round.RoundStart) << ", phase time " << phaseTimeMillis << "ms, round " << round.Round;
			auto& committeeData = pFsmShared->committeeData();
			committeeData.setCommitteeRound(round);
			committeeData.setCurrentBlockHeight(block.Height + Height(1));

			DelayAction(pFsmWeak, pFsmShared->timer(), 0u, [pFsmWeak] {
				TRY_GET_FSM()

				pFsmShared->processEvent(StageDetectionSucceeded{});
			});
		};
	}

	action CreateDefaultSelectCommitteeAction(
			const std::weak_ptr<WeightedVotingFsm>& pFsmWeak,
			extensions::ServiceState& state) {
		return [pFsmWeak, &state]() {
			TRY_GET_FSM()

			auto& committeeData = pFsmShared->committeeData();
			committeeData.setUnexpectedProposedBlockHeight(false);
			committeeData.setUnexpectedConfirmedBlockHeight(false);
			auto round = committeeData.committeeRound();
			auto pConfigHolder = state.pluginManager().configHolder();
			bool isInDbrbSystem = pFsmShared->dbrbProcess()->updateView(pConfigHolder, utils::FromTimePoint(round.RoundStart), committeeData.currentBlockHeight(), true);
			if (!isInDbrbSystem) {
				pFsmShared->processEvent(NotRegisteredInDbrbSystem{});
				return;
			}

			if (CommitteePhase::None == round.StartPhase)
				CATAPULT_THROW_RUNTIME_ERROR("committee start phase is not set")

			auto& committeeManager = state.pluginManager().getCommitteeManager(model::Block::Current_Version);
			if (committeeManager.committee().Round > round.Round)
				CATAPULT_THROW_RUNTIME_ERROR_2("invalid committee round", committeeManager.committee().Round, round.Round)

			const auto& config = pConfigHolder->Config(committeeData.currentBlockHeight()).Network;
			while (committeeManager.committee().Round < round.Round)
				committeeManager.selectCommittee(config);
			CATAPULT_LOG(debug) << "block " << committeeData.currentBlockHeight() << ": selected committee for round " << round.Round;
			committeeManager.logCommittee();

			const auto& committee = committeeManager.committee();
			committeeData.addCommitteeBootKey(committeeManager.getBootKey(committee.BlockProposer, config));
			for (const auto& key: committee.Cosigners)
				committeeData.addCommitteeBootKey(committeeManager.getBootKey(key, config));

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

			auto totalSumOfVotes = committeeManager.weight(committee.BlockProposer, config);
			for (const auto& cosigner : committee.Cosigners)
				committeeManager.add(totalSumOfVotes, committeeManager.weight(cosigner, config));
			committeeData.setTotalSumOfVotes(totalSumOfVotes);

			if (committeeData.committeeBootKeys().size() < config.CommitteeSize) {
				pFsmShared->processEvent(NotEnoughBootKeys{});
			} else {
				CATAPULT_LOG(debug) << "committee selection result: is block proposer = " << isBlockProposer << ", is cosigner = " << isCosigner << ", start phase = " << round.StartPhase << ", phase time = " << round.PhaseTimeMillis << "ms";
				pFsmShared->processEvent(CommitteeSelectionResult{ isBlockProposer, isCosigner, round.StartPhase });
			}
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
				pFsmShared->processEvent(BlockGenerationFailed{});
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
				committeeData.addValidatedProposedBlockSignature(pBlock->Signature);
				auto pPacket = ionet::CreateSharedPacket<ionet::Packet>(pBlock->Size);
				pPacket->Type = ionet::PacketType::Push_Proposed_Block;
				std::memcpy(static_cast<void*>(pPacket->Data()), pBlock.get(), pBlock->Size);
				pFsmShared->dbrbProcess()->broadcast(pPacket, committeeData.committeeBootKeys());
				pFsmShared->processEvent(BlockGenerationSucceeded{});
			} else {
				pFsmShared->processEvent(BlockGenerationFailed{});
			}
		};
	}

	action CreateDefaultWaitForProposalAction(const std::weak_ptr<WeightedVotingFsm>& pFsmWeak) {
		return [pFsmWeak]() {
			TRY_GET_FSM()

			auto phaseEndTimeMillis = GetPhaseEndTimeMillis(CommitteePhase::Propose, pFsmShared->committeeData().committeeRound().PhaseTimeMillis);
			DelayAction(pFsmWeak, pFsmShared->timer(), phaseEndTimeMillis, [pFsmWeak] {
				TRY_GET_FSM()

				auto& committeeData = pFsmShared->committeeData();
				if (committeeData.unexpectedProposedBlockHeight()) {
					pFsmShared->processEvent(UnexpectedBlockHeight{});
				} else if (committeeData.proposedBlock()) {
					pFsmShared->processEvent(ProposalReceived{});
				} else {
					pFsmShared->processEvent(ProposalNotReceived{});
				}
			});
		};
	}

	action CreateDefaultWaitForPrevotesAction(const std::weak_ptr<WeightedVotingFsm>& pFsmWeak) {
		return [pFsmWeak]() {
			TRY_GET_FSM()

			auto& committeeData = pFsmShared->committeeData();
			if (committeeData.sumOfPrevotesSufficient()) {
				pFsmShared->processEvent(SumOfPrevotesSufficient{});
			} else {
				auto future = committeeData.startWaitForPrevotes();
				auto phaseEndTimeMillis = GetPhaseEndTimeMillis(CommitteePhase::Prevote, committeeData.committeeRound().PhaseTimeMillis);
				auto timeout = committeeData.committeeRound().RoundStart + std::chrono::milliseconds(phaseEndTimeMillis);
				try {
					auto status = future.wait_until(timeout);
					if (std::future_status::ready == status && future.get()) {
						pFsmShared->processEvent(SumOfPrevotesSufficient{});
						return;
					}
				} catch (std::exception const& error) {
					CATAPULT_LOG(warning) << "error waiting for prevotes: " << error.what();
				} catch (...) {
					CATAPULT_LOG(warning) << "error waiting for prevotes: unknown error";
				}

				committeeData.calculateSumOfVotes();
				if (committeeData.sumOfPrevotesSufficient()) {
					pFsmShared->processEvent(SumOfPrevotesSufficient{});
				} else {
					pFsmShared->processEvent(SumOfPrevotesInsufficient{});
				}
			}
		};
	}

	action CreateDefaultWaitForPrecommitsAction(const std::weak_ptr<WeightedVotingFsm>& pFsmWeak) {
		return [pFsmWeak]() {
			TRY_GET_FSM()

			auto& committeeData = pFsmShared->committeeData();
			if (committeeData.sumOfPrevotesSufficient() && committeeData.sumOfPrecommitsSufficient()) {
				pFsmShared->processEvent(SumOfPrecommitsSufficient{});
			} else {
				auto future = committeeData.startWaitForPrecommits();
				auto phaseEndTimeMillis = GetPhaseEndTimeMillis(CommitteePhase::Precommit, committeeData.committeeRound().PhaseTimeMillis);
				auto timeout = committeeData.committeeRound().RoundStart + std::chrono::milliseconds(phaseEndTimeMillis);
				try {
					auto status = future.wait_until(timeout);
					if (std::future_status::ready == status && future.get() && committeeData.sumOfPrevotesSufficient()) {
						pFsmShared->processEvent(SumOfPrecommitsSufficient{});
						return;
					}
				} catch (std::exception const& error) {
					CATAPULT_LOG(warning) << "error waiting for precommits: " << error.what();
				} catch (...) {
					CATAPULT_LOG(warning) << "error waiting for precommits: unknown error";
				}

				committeeData.calculateSumOfVotes();
				if (committeeData.sumOfPrevotesSufficient() && committeeData.sumOfPrecommitsSufficient()) {
					pFsmShared->processEvent(SumOfPrecommitsSufficient{});
				} else {
					pFsmShared->processEvent(SumOfPrecommitsInsufficient{});
				}
			}
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

				VoteMap votes;
				for (const auto* pKeyPair : committeeData.localCommittee()) {
					CommitteeMessage message;
					message.Type = MessageType;
					message.BlockHash = committeeData.proposedBlockHash();
					auto& cosignature = message.BlockCosignature;
					cosignature.Signer = pKeyPair->publicKey();
					model::CosignBlockHeader(*pKeyPair, *pProposedBlock, cosignature.Signature);
					crypto::Sign(*pKeyPair, CommitteeMessageDataBuffer(message), message.MessageSignature);
					votes.emplace(cosignature.Signer, message);
				}

				CATAPULT_LOG(debug) << "added " << votes.size() << " " << Phase << " votes";
				if (votes.empty())
					return;

				auto pPacket = ionet::CreateSharedPacket<TPacket>(utils::checked_cast<size_t, uint32_t>(sizeof(CommitteeMessage) * votes.size()));
				pPacket->MessageCount = utils::checked_cast<size_t, uint8_t>(votes.size());

				auto* pMessage = reinterpret_cast<CommitteeMessage*>(pPacket.get() + 1);
				auto index = 0u;
				for (const auto& pair : votes)
					pMessage[index++] = pair.second;

				pFsmShared->dbrbProcess()->broadcast(pPacket, committeeData.committeeBootKeys());
			};
		}
	}

	action CreateDefaultAddPrevoteAction(const std::weak_ptr<WeightedVotingFsm>& pFsmWeak) {
		return CreateAddVoteAction<PushPrevoteMessagesRequest, CommitteeMessageType::Prevote, CommitteePhase::Prevote>(pFsmWeak);
	}

	action CreateDefaultAddPrecommitAction(const std::weak_ptr<WeightedVotingFsm>& pFsmWeak) {
		return CreateAddVoteAction<PushPrecommitMessagesRequest, CommitteeMessageType::Precommit, CommitteePhase::Precommit>(pFsmWeak);
	}

	action CreateDefaultUpdateConfirmedBlockAction(
		const std::weak_ptr<WeightedVotingFsm>& pFsmWeak,
			extensions::ServiceState& state) {
		return [pFsmWeak, &state]() {
			TRY_GET_FSM()

			auto& committeeData = pFsmShared->committeeData();
			auto pProposedBlock = committeeData.proposedBlock();
			if (!pProposedBlock)
				CATAPULT_THROW_RUNTIME_ERROR("update confirmed block failed, no proposed block")

			// Collect cosignatures.
			auto votes = committeeData.precommits();
			std::vector<model::Cosignature> cosignatures;
			cosignatures.reserve(votes.size());
			const auto& blockProposer = state.pluginManager().getCommitteeManager(model::Block::Current_Version).committee().BlockProposer;
			for (const auto &pair : votes) {
				if (pair.first != blockProposer)
					cosignatures.push_back(pair.second.BlockCosignature);
			}

			// Broadcast the confirmed block.
			auto cosignaturesSize = cosignatures.size() * sizeof(model::Cosignature);
			auto blockSize = pProposedBlock->Size + cosignaturesSize;
			auto pPacket = ionet::CreateSharedPacket<ionet::Packet>(blockSize);
			pPacket->Type = ionet::PacketType::Push_Confirmed_Block;
			std::memcpy(static_cast<void*>(pPacket->Data()), pProposedBlock.get(), pProposedBlock->Size);
			std::memcpy(static_cast<void*>(pPacket->Data() + pProposedBlock->Size), cosignatures.data(), cosignaturesSize);
			reinterpret_cast<model::Block*>(pPacket->Data())->Size = blockSize;
			const auto& pDbrbProcess = pFsmShared->dbrbProcess();
			pDbrbProcess->broadcast(pPacket, pDbrbProcess->currentView().Data);
		};
	}

	action CreateDefaultWaitForConfirmedBlockAction(const std::weak_ptr<WeightedVotingFsm>& pFsmWeak, extensions::ServiceState& state) {
		return [pFsmWeak, &state]() {
			TRY_GET_FSM()

			auto& committeeData = pFsmShared->committeeData();
			if (committeeData.confirmedBlock()) {
				pFsmShared->processEvent(ConfirmedBlockReceived{});
			} else {
				auto future = committeeData.startWaitForConfirmedBlock();
				auto phaseEndTimeMillis = GetPhaseEndTimeMillis(CommitteePhase::Commit, committeeData.committeeRound().PhaseTimeMillis);
				auto timeout = committeeData.committeeRound().RoundStart + std::chrono::milliseconds(phaseEndTimeMillis);
				try {
					auto status = future.wait_until(timeout);
					if (std::future_status::ready == status && future.get()) {
						pFsmShared->processEvent(ConfirmedBlockReceived{});
						return;
					}
				} catch (std::exception const& error) {
					CATAPULT_LOG(warning) << "error waiting for confirmed block: " << error.what();
				} catch (...) {
					CATAPULT_LOG(warning) << "error waiting for confirmed block: unknown error";
				}

				if (committeeData.unexpectedConfirmedBlockHeight()) {
					pFsmShared->processEvent(UnexpectedBlockHeight{});
				} else {
					pFsmShared->processEvent(ConfirmedBlockNotReceived{});
				}
			}
		};
	}

	action CreateDefaultCommitConfirmedBlockAction(
			const std::weak_ptr<WeightedVotingFsm>& pFsmWeak,
			const consumer<model::BlockRange&&, const disruptor::ProcessingCompleteFunc&>& rangeConsumer,
			extensions::ServiceState& state) {
		return [pFsmWeak, rangeConsumer, &state]() {
			TRY_GET_FSM()

			bool success = false;
			auto& committeeData = pFsmShared->committeeData();
			auto pConfirmedBlock = committeeData.confirmedBlock();
			{
				// Commit block.
				std::lock_guard<std::mutex> guard(pFsmShared->mutex());

				auto pPromise = std::make_shared<thread::promise<bool>>();
				rangeConsumer(model::BlockRange::FromEntity(pConfirmedBlock), [pPromise, pConfirmedBlock](auto, const auto& result) {
					bool success = (disruptor::CompletionStatus::Aborted != result.CompletionStatus);
					if (success) {
						CATAPULT_LOG(info) << "successfully committed confirmed block produced by " << pConfirmedBlock->Signer;
					} else {
						auto validationResult = static_cast<validators::ValidationResult>(result.CompletionCode);
						CATAPULT_LOG_LEVEL(MapToLogLevel(validationResult)) << "confirmed block commit failed due to " << validationResult;
					}

					pPromise->set_value(std::move(success));
				});

				success = pPromise->get_future().get();
			}

			DelayAction(pFsmWeak, pFsmShared->timer(), GetPhaseEndTimeMillis(CommitteePhase::Commit, committeeData.committeeRound().PhaseTimeMillis), [pFsmWeak, success, &state] {
				TRY_GET_FSM()

				const auto& maxChainHeight = state.maxChainHeight();
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
			chain::IncreasePhaseTime(nextPhaseTimeMillis, pConfigHolder->Config(committeeData.currentBlockHeight()).Network);
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
			extensions::ServiceState& state) {
		return [pFsmWeak, &state]() {
			TRY_GET_FSM()

			auto& committeeData = pFsmShared->committeeData();
			auto currentRound = committeeData.committeeRound();
			pFsmShared->resetCommitteeData();
			state.pluginManager().getCommitteeManager(model::Block::Current_Version).reset();

			auto nextRoundStart = currentRound.RoundStart + std::chrono::milliseconds(GetPhaseEndTimeMillis(CommitteePhase::Commit, currentRound.PhaseTimeMillis));
			uint64_t nextPhaseTimeMillis = currentRound.PhaseTimeMillis;
			chain::DecreasePhaseTime(nextPhaseTimeMillis, state.pluginManager().config(committeeData.currentBlockHeight() + Height(1)));
			committeeData.incrementCurrentBlockHeight();
			committeeData.setCommitteeRound(CommitteeRound{
				0u,
				CommitteePhase::Propose,
				nextRoundStart,
				nextPhaseTimeMillis
			});
		};
	}
}}