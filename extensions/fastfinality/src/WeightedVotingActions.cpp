/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "WeightedVotingActions.h"
#include "WeightedVotingFsm.h"
#include "catapult/api/LocalChainApi.h"
#include "catapult/api/RemoteChainApi.h"
#include "catapult/extensions/LocalNodeChainScore.h"
#include "catapult/extensions/PluginUtils.h"
#include "catapult/chain/BlockDifficultyScorer.h"
#include "catapult/chain/ChainUtils.h"
#include "catapult/chain/RemoteApiForwarder.h"
#include "catapult/crypto/Signer.h"
#include "catapult/extensions/ExecutionConfigurationFactory.h"
#include "catapult/extensions/NodeInteractionUtils.h"
#include "catapult/harvesting_core/UnlockedAccounts.h"
#include "catapult/ionet/PacketPayloadFactory.h"
#include "catapult/model/BlockUtils.h"
#include "catapult/utils/StackLogger.h"
#include "catapult/validators/AggregateEntityValidator.h"

namespace catapult { namespace fastfinality {

	namespace {
		bool PeerNumberSufficient(
				uint32_t peerNumber,
				const model::NetworkConfiguration& config) {
			return peerNumber >= std::floor(config.CommitteeApproval * config.CommitteeSize);
		}

		template<typename TValue, typename TValueArray, typename TValueAdapter>
		auto FindMostFrequentValue(const TValueArray& values, const TValueAdapter& adapter) {
			TValue mostFrequentValue;
			uint32_t maxFrequency = 0;
			std::map<TValue, uint32_t> valueFrequencies;
			for (const auto& valueWrapper : values) {
				const auto& value = adapter(valueWrapper);
				auto frequency = ++valueFrequencies[value];
				if (maxFrequency < frequency) {
					maxFrequency = frequency;
					mostFrequentValue = value;
				}
			}

			return std::make_pair(mostFrequentValue, maxFrequency);
		}
	}

	action CreateDefaultCheckLocalChainAction(
			const std::shared_ptr<WeightedVotingFsm>& pFsm,
			const extensions::RemoteChainHeightsRetriever& retriever,
			const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder,
			const supplier<Height>& localHeightSupplier) {
		return [pFsm, retriever, pConfigHolder, localHeightSupplier]() {
			pFsm->resetChainSyncData();

			const auto& config = pConfigHolder->Config().Network;
			return retriever(config.CommitteeSize).then([localHeightSupplier, pFsm, &config, pConfigHolder](auto&& heightsFuture) {
				auto heights = heightsFuture.get();
				auto pair = FindMostFrequentValue<Height>(heights, [] (const Height& height) { return height; });
				if (PeerNumberSufficient(pair.second, config)) {
					auto localHeight = localHeightSupplier();
					auto networkHeight = pair.first;

					auto& chainSyncData = pFsm->chainSyncData();
					chainSyncData.LocalHeight = localHeight;
					chainSyncData.NetworkHeight = networkHeight;

					if (networkHeight < localHeight) {
						pFsm->processEvent(NetworkHeightLessThanLocal{});
					} else if (networkHeight > localHeight) {
						pFsm->processEvent(NetworkHeightGreaterThanLocal{});
					} else {
						pFsm->processEvent(NetworkHeightEqualToLocal{});
					}
				} else {
					pFsm->processEvent(NetworkHeightDetectionFailure{});
				}
			});
		};
	}

	action CreateDefaultResetLocalChainAction() {
		CATAPULT_THROW_RUNTIME_ERROR("local chain is invalid and needs to be reset");
	}

	action CreateDefaultSelectPeersAction(
			const std::shared_ptr<WeightedVotingFsm>& pFsm,
			const RemoteBlockHashesIoRetriever& retriever,
			const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder) {
		return [pFsm, retriever, pConfigHolder]() {
			const auto& config = pConfigHolder->Config();
			auto& chainSyncData = pFsm->chainSyncData();
			auto targetHeight = std::min(chainSyncData.NetworkHeight, chainSyncData.LocalHeight + Height(config.Node.MaxBlocksPerSyncAttempt));
			return retriever(config.Network.CommitteeSize, targetHeight).then([pFsm, &config](auto&& hashIoPairsFuture) {
				auto hashIoPairs = hashIoPairsFuture.get();
				auto pair = FindMostFrequentValue<Hash256>(hashIoPairs, [] (const auto& pair) { return pair.first; });
				if (PeerNumberSufficient(pair.second, config.Network)) {
					const auto& blockHash = pair.first;
					auto& packetIoPairs = pFsm->chainSyncData().PacketIoPairs;
					for (const auto& hashIoPair : hashIoPairs) {
						if (hashIoPair.first == blockHash)
							packetIoPairs.push_back(hashIoPair.second);
					}
					pFsm->processEvent(PeersSelectionSucceeded{});
				} else {
					pFsm->processEvent(PeersSelectionFailed{});
				}
			});
		};
	}

	namespace {
		chain::ChainSynchronizerConfiguration CreateChainSynchronizerConfiguration(const config::BlockchainConfiguration& config) {
			chain::ChainSynchronizerConfiguration chainSynchronizerConfig;
			chainSynchronizerConfig.MaxBlocksPerSyncAttempt = config.Node.MaxBlocksPerSyncAttempt;
			chainSynchronizerConfig.MaxChainBytesPerSyncAttempt = config.Node.MaxChainBytesPerSyncAttempt.bytes32();
			return chainSynchronizerConfig;
		}
	}

	action CreateDefaultDownloadBlocksAction(
			const std::shared_ptr<WeightedVotingFsm>& pFsm,
			extensions::ServiceState& state) {
		return [&state, pFsm]() {
			const auto& config = state.config();
			auto chainSynchronizer = chain::CreateChainSynchronizer(
				api::CreateLocalChainApi(state.storage(), [&score = state.score()]() {
					return score.get();
				}),
				CreateChainSynchronizerConfiguration(config),
				state,
				state.hooks().completionAwareBlockRangeConsumerFactory()(disruptor::InputSource::Remote_Pull));

			for (const auto& pPackerIoPair : pFsm->chainSyncData().PacketIoPairs) {
				auto syncTimeout = state.config().Node.SyncTimeout;
				std::shared_ptr<net::PacketIoPicker> pPacketIoPicker = nullptr;
				chain::RemoteApiForwarder forwarder(pPacketIoPicker, state.pluginManager().transactionRegistry(), syncTimeout, "block downloader", pPackerIoPair);

				auto result = forwarder.processSync(chainSynchronizer, api::CreateRemoteChainApi).get();
				extensions::IncrementNodeInteraction(state.nodes(), result);
				if (ionet::NodeInteractionResultCode::Success == result.Code) {
					pFsm->processEvent(DownloadBlocksSucceeded{});
					return;
				}
			}

			pFsm->processEvent(DownloadBlocksFailed{});
		};
	}

	action CreateDefaultDetectStageAction(
			const std::shared_ptr<WeightedVotingFsm>& pFsm,
			const RemoteCommitteeStagesRetriever& retriever,
			const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder,
			const chain::TimeSupplier& timeSupplier,
			const model::BlockElementSupplier& lastBlockElementSupplier) {
		return [pFsm, retriever, pConfigHolder, timeSupplier, lastBlockElementSupplier]() {
			pFsm->resetChainSyncData();
			pFsm->resetCommitteeData();

			const auto& config = pConfigHolder->Config().Network;
			return retriever(config.CommitteeSize).then([pFsm, &config, timeSupplier, lastBlockElementSupplier](auto&& stagesFuture) {
				auto stages = stagesFuture.get();
				auto pair = FindMostFrequentValue<CommitteeStage>(stages, [] (const auto& stage) { return stage; });

				auto& committeeData = pFsm->committeeData();
				auto pLastBlockElement = lastBlockElementSupplier();
				const auto& block = pLastBlockElement->Block;
				auto lastBlockHeight = block.Height;
				if (PeerNumberSufficient(pair.second, config)) {
					committeeData.setCommitteeStage(pair.first);

					pFsm->processEvent(StageDetectionSucceeded{});
				} else if (Height(1) == lastBlockHeight) {
					committeeData.setCommitteeStage(CommitteeStage{
						0u,
						CommitteePhase::Prevote,
						utils::ToTimePoint(timeSupplier()),
						config.CommitteePhaseTime.millis()
					});

					pFsm->processEvent(StageDetectionSucceeded{});
				} else {
					pFsm->processEvent(StageDetectionFailed{});
				}
			});
		};
	}

	action CreateDefaultSelectCommitteeAction(
			const std::shared_ptr<WeightedVotingFsm>& pFsm,
			chain::CommitteeManager& committeeManager,
			const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder) {
		return [pFsm, &committeeManager, pConfigHolder]() {
			auto& committeeData = pFsm->committeeData();
			auto stage = committeeData.committeeStage();
			if (CommitteePhase::None == stage.Phase)
				CATAPULT_THROW_RUNTIME_ERROR("committee phase is not set");

			if (0u == stage.Round) {
				committeeManager.reset();
			} else {
				const auto& config = pConfigHolder->Config().Network;
				while (committeeManager.selectCommittee(config).Round != stage.Round);
			}

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

			pFsm->processEvent(CommitteeSelectionResult{ isBlockProposer, stage.Phase });
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

		void DelayAction(const std::shared_ptr<WeightedVotingFsm>& pFsm, uint64_t delay, action callback) {
			auto& timer = pFsm->timer();
			auto& committeeData = pFsm->committeeData();
			timer.expires_at(committeeData.committeeStage().RoundStart + std::chrono::milliseconds(delay));
			timer.async_wait([callback](const boost::system::error_code& ec) {
				if (ec) {
					if (ec == boost::asio::error::operation_aborted)
						return;

					CATAPULT_THROW_EXCEPTION(boost::system::system_error(ec));
				}

				callback();
			});
		}
	}

	action CreateDefaultProposeBlockAction(
			const std::shared_ptr<WeightedVotingFsm>& pFsm,
			const cache::CatapultCache& cache,
			const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder,
			const harvesting::BlockGenerator& blockGenerator,
			const model::BlockElementSupplier& lastBlockElementSupplier,
			const extensions::PacketPayloadSink& packetPayloadSink) {
		return [pFsm, &cache, pConfigHolder, blockGenerator, lastBlockElementSupplier, packetPayloadSink]() {
			auto& committeeData = pFsm->committeeData();
			committeeData.setProposedBlock(nullptr);
			auto committeeStage = committeeData.committeeStage();
			NextBlockContext context(*lastBlockElementSupplier(), utils::FromTimePoint(committeeStage.RoundStart));
			const auto& config = pConfigHolder->Config(context.Height);
			if (!context.tryCalculateDifficulty(cache.sub<cache::BlockDifficultyCache>(), config.Network)) {
				CATAPULT_LOG(debug) << "skipping block propose attempt due to error calculating difficulty";
				pFsm->processEvent(BlockProposingFailed{});
			}

			utils::StackLogger stackLogger("generating candidate block", utils::LogLevel::Debug);
			auto pBlockHeader = model::CreateBlock(context.ParentContext, config.Immutable.NetworkIdentifier, committeeData.blockProposer()->publicKey(), {});
			pBlockHeader->Difficulty = context.Difficulty;
			pBlockHeader->Timestamp = context.Timestamp;
			pBlockHeader->Beneficiary = committeeData.beneficiary();
			pBlockHeader->CommitteePhaseTime = committeeStage.PhaseTimeMillis;
			auto pBlock = utils::UniqueToShared(blockGenerator(*pBlockHeader, config.Network.MaxTransactionsPerBlock));
			if (pBlock) {
				model::SignBlockHeader(*committeeData.blockProposer(), *pBlock);
				packetPayloadSink(ionet::PacketPayloadFactory::FromEntity(ionet::PacketType::Push_Proposed_Block, pBlock));
				committeeData.setProposedBlock(pBlock);

				DelayAction(pFsm, committeeStage.PhaseTimeMillis, [pFsm] {
					pFsm->processEvent(BlockProposingSucceeded{});
				});
			} else {
				pFsm->processEvent(BlockProposingFailed{});
			}
		};
	}

	namespace {
		void TryGetProposedBlock(
				const std::shared_ptr<WeightedVotingFsm>& pFsm,
				const RemoteProposedBlockRetriever& retriever,
				const model::NetworkConfiguration& config,
				const extensions::PacketPayloadSink& packetPayloadSink) {
			retriever(config.CommitteeSize).then([pFsm, &config, packetPayloadSink](auto&& blocksFuture) {
				auto blocks = blocksFuture.get();
				std::vector<std::pair<Hash256, std::shared_ptr<model::Block>>> hashBlockPairs;
				for (const auto& pBlock : blocks)
					hashBlockPairs.push_back(std::make_pair(model::CalculateHash(*pBlock), pBlock));
				auto frequencyPair = FindMostFrequentValue<Hash256>(hashBlockPairs, [] (const auto& pair) { return pair.first; });

				if (PeerNumberSufficient(frequencyPair.second, config)) {
					for (const auto& hashBlockPair : hashBlockPairs) {
						if (hashBlockPair.first == frequencyPair.first) {
							pFsm->committeeData().setProposedBlock(hashBlockPair.second);
							packetPayloadSink(ionet::PacketPayloadFactory::FromEntity(ionet::PacketType::Push_Proposed_Block, hashBlockPair.second));
							return;
						}
					}
				}
			});
		}
	}

	action CreateDefaultWaitForProposalAction(
			const std::shared_ptr<WeightedVotingFsm>& pFsm,
			const RemoteProposedBlockRetriever& retriever,
			const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder,
			const extensions::PacketPayloadSink& packetPayloadSink) {
		return [pFsm, retriever, pConfigHolder, packetPayloadSink]() {
			if (!pFsm->committeeData().proposedBlock())
				TryGetProposedBlock(pFsm, retriever, pConfigHolder->Config().Network, packetPayloadSink);

			DelayAction(pFsm, pFsm->committeeData().committeeStage().PhaseTimeMillis, [pFsm] {
				const auto& committeeData = pFsm->committeeData();
				if (committeeData.proposalMultiple()) {
					pFsm->processEvent(MultipleProposal{});
				} else if (!committeeData.proposedBlock()) {
					pFsm->processEvent(ProposalNotReceived{});
				} else {
					pFsm->processEvent(ProposalReceived{});
				}
			});
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
				const std::shared_ptr<WeightedVotingFsm>& pFsm,
				extensions::ServiceState& state,
				const model::BlockElementSupplier& lastBlockElementSupplier,
				const std::shared_ptr<thread::IoThreadPool>& pValidatorPool) {
			auto blockConsumers = CreateBlockConsumers(state, lastBlockElementSupplier, pValidatorPool);
			auto pProposedBlock = pFsm->committeeData().proposedBlock();
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
			const std::shared_ptr<WeightedVotingFsm>& pFsm,
			extensions::ServiceState& state,
			const model::BlockElementSupplier& lastBlockElementSupplier,
			const std::shared_ptr<thread::IoThreadPool>& pValidatorPool) {
		return [pFsm, &state, lastBlockElementSupplier, pValidatorPool]() {
			if (ValidateProposedBlock(pFsm, state, lastBlockElementSupplier, pValidatorPool)) {
				pFsm->processEvent(ProposalValid{});
			} else {
				pFsm->processEvent(ProposalInvalid{});
			}
		};
	}

	namespace {
		void SetPhase(std::shared_ptr<WeightedVotingFsm> pFsm, CommitteePhase phase) {
			auto& committeeData = pFsm->committeeData();
			auto stage = committeeData.committeeStage();
			stage.Phase = phase;
			committeeData.setCommitteeStage(stage);
		}

		bool IsSumOfVotesSufficient(const VoteMap& votes, double committeeApproval, double totalSumOfVotes, const chain::CommitteeManager& committeeManager) {
			double sum = 0.0;
			for (const auto& pair : votes) {
				sum += committeeManager.weight(pair.first);
			}
			return sum >= committeeApproval * totalSumOfVotes;
		}

		template<typename TSumOfVotesSufficient, typename TSumOfVotesInsufficient>
		action CreateWaitForVotesAction(
			const std::shared_ptr<WeightedVotingFsm>& pFsm,
			const plugins::PluginManager& pluginManager,
			supplier<VoteMap> voteSupplier,
			uint8_t phaseNumber,
			CommitteePhase nextPhase) {
			return [pFsm, &pluginManager, voteSupplier, phaseNumber, nextPhase]() {
				const auto& pConfigHolder = pluginManager.configHolder();
				const auto& committeeManager = pluginManager.getCommitteeManager();

				DelayAction(
					pFsm,
					phaseNumber * pFsm->committeeData().committeeStage().PhaseTimeMillis,
					[pFsm, pConfigHolder, &committeeManager, voteSupplier, nextPhase] {
						SetPhase(pFsm, nextPhase);
						const auto& committeeData = pFsm->committeeData();
						auto committeeApproval = pConfigHolder->Config().Network.CommitteeApproval;
						if (IsSumOfVotesSufficient(voteSupplier(), committeeApproval, committeeData.totalSumOfVotes(), committeeManager)) {
							pFsm->processEvent(TSumOfVotesSufficient{});
						} else {
							pFsm->processEvent(TSumOfVotesInsufficient{});
						}
					}
				);
			};
		}
	}

	action CreateDefaultWaitForPrevotesAction(
			const std::shared_ptr<WeightedVotingFsm>& pFsm,
			const plugins::PluginManager& pluginManager) {
		const auto& committeeData = pFsm->committeeData();
		return CreateWaitForVotesAction<SumOfPrevotesSufficient, SumOfPrevotesInsufficient>(
			pFsm, pluginManager, [&committeeData](){ return committeeData.prevotes(); }, 2, CommitteePhase::Precommit);
	}

	action CreateDefaultWaitForPrecommitsAction(
			const std::shared_ptr<WeightedVotingFsm>& pFsm,
			const plugins::PluginManager& pluginManager) {
		const auto& committeeData = pFsm->committeeData();
		return CreateWaitForVotesAction<SumOfPrecommitsSufficient, SumOfPrecommitsInsufficient>(
			pFsm, pluginManager, [&committeeData](){ return committeeData.precommits(); }, 3, CommitteePhase::Commit);
	}

	namespace {
		template<typename TPacket>
		action CreateBroadcastVoteAction(
			const std::shared_ptr<WeightedVotingFsm>& pFsm,
			const extensions::PacketPayloadSink& packetPayloadSink,
			consumer<model::Cosignature&, const crypto::KeyPair*> setBlockCosignature,
			CommitteePhase phase) {
			return [pFsm, packetPayloadSink, setBlockCosignature, phase]() {
				SetPhase(pFsm, phase);
				auto& committeeData = pFsm->committeeData();
				auto pPacket = ionet::CreateSharedPacket<TPacket>();
				pPacket->Message.BlockHash = committeeData.proposedBlockHash();
				for (const auto* pKeyPair : committeeData.localCommittee()) {
					auto& cosignature = pPacket->Message.BlockCosignature;
					cosignature.Signer = pKeyPair->publicKey();
					setBlockCosignature(pPacket->Message.BlockCosignature, pKeyPair);

					crypto::Sign(*pKeyPair, CommitteeMessageDataBuffer(*pPacket), pPacket->MessageSignature);
					packetPayloadSink(ionet::PacketPayload(pPacket));
				}
			};
		}
	}

	action CreateDefaultBroadcastPrevoteAction(
			const std::shared_ptr<WeightedVotingFsm>& pFsm,
			const extensions::PacketPayloadSink& packetPayloadSink) {
		return CreateBroadcastVoteAction<PrevoteMessagePacket>(
			pFsm,
			packetPayloadSink,
			[pFsm](auto& cosignature, const auto* pKeyPair) {
				auto& committeeData = pFsm->committeeData();
				auto pProposedBlock = committeeData.proposedBlock();
				if (!pProposedBlock)
					CATAPULT_THROW_RUNTIME_ERROR("broadcast prevote message failed, no proposed block");
				model::CosignBlockHeader(*pKeyPair, *pProposedBlock, cosignature.Signature);
				committeeData.addPrevote(cosignature.Signer, cosignature.Signature);
			},
			CommitteePhase::Prevote);
	}

	action CreateDefaultBroadcastPrecommitAction(
			const std::shared_ptr<WeightedVotingFsm>& pFsm,
			const extensions::PacketPayloadSink& packetPayloadSink) {
		return CreateBroadcastVoteAction<PrecommitMessagePacket>(
			pFsm,
			packetPayloadSink,
			[pFsm](auto& cosignature, const auto*) {
				auto& committeeData = pFsm->committeeData();
				cosignature.Signature = committeeData.getPrevote(cosignature.Signer);
				committeeData.addPrecommit(cosignature.Signer, cosignature.Signature);
			},
			CommitteePhase::Precommit);
	}

	action CreateDefaultCommitConfirmedBlockAction(
			const std::shared_ptr<WeightedVotingFsm>& pFsm,
			consumer<model::BlockRange&&, const disruptor::ProcessingCompleteFunc&> rangeConsumer) {
		return [pFsm, &rangeConsumer]() {
			auto& committeeData = pFsm->committeeData();
			auto pProposedBlock = committeeData.proposedBlock();

			// Insert cosignatures.
			auto votes = committeeData.precommits();
			std::vector<model::Cosignature> cosignatures;
			cosignatures.reserve(votes.size());
			for (const auto& pair : votes)
				cosignatures.push_back({ pair.first, pair.second });
			auto cosignaturesSize = cosignatures.size() * sizeof(model::Cosignature);
			auto pBlock = utils::MakeSharedWithSize<model::Block>(pProposedBlock->Size + cosignaturesSize);
			std::memcpy(static_cast<void*>(pBlock.get()), pProposedBlock.get(), pProposedBlock->Size);
			std::memcpy(static_cast<void*>(pBlock->CosignaturesPtr()), cosignatures.data(), cosignaturesSize);

			// Commit block.
			thread::promise<bool> promise;
			auto future = promise.get_future();
			rangeConsumer(model::BlockRange::FromEntity(pBlock), [&promise](auto, const disruptor::ConsumerCompletionResult& result) {
				bool success = (disruptor::CompletionStatus::Aborted != result.CompletionStatus);
				if (success) {
					CATAPULT_LOG(trace) << "successfully committed proposed block";
				} else {
					auto validationResult = static_cast<validators::ValidationResult>(result.CompletionCode);
					CATAPULT_LOG_LEVEL(MapToLogLevel(validationResult))
						<< "proposed block commit failed due to " << validationResult;
				}

				promise.set_value(std::move(success));
			});

			bool success = future.get();
			if (success) {
				pFsm->processEvent(CommitBlockSucceeded{});
			} else {
				pFsm->processEvent(CommitBlockFailed{});
			}
		};
	}

	action CreateDefaultIncrementRoundAction(
			const std::shared_ptr<WeightedVotingFsm>& pFsm,
			const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder) {
		return [pFsm, pConfigHolder]() {
			auto currentStage = pFsm->committeeData().committeeStage();
			pFsm->resetCommitteeData();

			uint16_t nextRound = currentStage.Round + 1u;
			auto nextRoundStart = currentStage.RoundStart + std::chrono::milliseconds(3 * currentStage.PhaseTimeMillis);
			uint64_t nextPhaseTimeMillis = currentStage.PhaseTimeMillis * pConfigHolder->Config().Network.CommitteeTimeAdjustment;
			auto& committeeData = pFsm->committeeData();
			committeeData.setCommitteeStage(CommitteeStage{
				nextRound,
				CommitteePhase::Propose,
				nextRoundStart,
				nextPhaseTimeMillis
			});
		};
	}

	action CreateDefaultResetRoundAction(
			const std::shared_ptr<WeightedVotingFsm>& pFsm,
			const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder) {
		return [pFsm, pConfigHolder]() {
			auto currentStage = pFsm->committeeData().committeeStage();
			pFsm->resetCommitteeData();

			auto nextRoundStart = currentStage.RoundStart + std::chrono::milliseconds(3 * currentStage.PhaseTimeMillis);
			uint64_t nextPhaseTimeMillis = currentStage.PhaseTimeMillis / pConfigHolder->Config().Network.CommitteeTimeAdjustment;
			auto& committeeData = pFsm->committeeData();
			committeeData.setCommitteeStage(CommitteeStage{
				0u,
				CommitteePhase::Propose,
				nextRoundStart,
				nextPhaseTimeMillis
			});
		};
	}

	action CreateDefaultWaitForRoundEndAction(
		const std::shared_ptr<WeightedVotingFsm>& pFsm) {
		return [pFsm]() {
			SetPhase(pFsm, CommitteePhase::Commit);
			DelayAction(pFsm, 3 * pFsm->committeeData().committeeStage().PhaseTimeMillis, [pFsm] {
				pFsm->resetCommitteeData();
				pFsm->processEvent(RoundEnded{});
			});
		};
	}
}}