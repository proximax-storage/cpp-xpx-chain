/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "WeightedVotingService.h"
#include "WeightedVotingFsm.h"
#include "catapult/api/RemoteChainApi.h"
#include "catapult/api/RemoteRequestDispatcher.h"
#include "catapult/crypto/KeyUtils.h"
#include "catapult/extensions/ExecutionConfigurationFactory.h"
#include "catapult/extensions/ServiceLocator.h"
#include "catapult/harvesting_core/HarvestingUtFacadeFactory.h"
#include "catapult/harvesting_core/UnlockedAccounts.h"
#include "catapult/io/BlockStorageCache.h"
#include "catapult/thread/MultiServicePool.h"

namespace catapult { namespace fastfinality {

	namespace {
		std::shared_ptr<harvesting::UnlockedAccounts> CreateUnlockedAccounts(const harvesting::HarvestingConfiguration& config) {
			auto pUnlockedAccounts = std::make_shared<harvesting::UnlockedAccounts>(config.MaxUnlockedAccounts);
			if (config.IsAutoHarvestingEnabled) {
				auto keyPair = crypto::KeyPair::FromString(config.HarvestKey);
				auto publicKey = keyPair.publicKey();

				auto unlockResult = pUnlockedAccounts->modifier().add(std::move(keyPair));
				CATAPULT_LOG(info)
						<< "Added account " << publicKey
						<< " for harvesting with result " << unlockResult;
			}

			return pUnlockedAccounts;
		}

		auto CreateRemoteBlockHashesIoRetriever(const net::PacketIoPickerContainer& packetIoPickers) {
			return [&packetIoPickers](auto numPeers, const Height& height) {
				std::vector<thread::future<std::pair<Hash256, std::shared_ptr<ionet::NodePacketIoPair>>>> futures;
				auto timeout = utils::TimeSpan::FromSeconds(5);

				auto packetIoPairs = packetIoPickers.pickMultiple(numPeers, timeout, ionet::NodeRoles::Peer);
				if (packetIoPairs.empty()) {
					CATAPULT_LOG(warning) << "could not find any peer for pulling block hashes";
					return thread::make_ready_future(std::vector<std::pair<Hash256, std::shared_ptr<ionet::NodePacketIoPair>>>());
				}

				for (const auto& packetIoPair : packetIoPairs) {
					auto pPacketIoPair = std::make_shared<ionet::NodePacketIoPair>(packetIoPair);
					auto pChainApi = api::CreateRemoteChainApiWithoutRegistry(*pPacketIoPair->io());
					futures.push_back(pChainApi->hashesFrom(height, 1u).then([pPacketIoPair](auto&& hashesFuture) {
						auto hash = *hashesFuture.get().cbegin();
						return std::make_pair(hash, pPacketIoPair);
					}));
				}

				return thread::when_all(std::move(futures)).then([](auto&& completedFutures) {
					return thread::get_all_ignore_exceptional(completedFutures.get());
				});
			};
		}

		auto CreateRemoteCommitteeStagesRetriever(const net::PacketIoPickerContainer& packetIoPickers) {
			return [&packetIoPickers](auto numPeers) {
				std::vector<thread::future<CommitteeStage>> committeeStageFutures;
				auto timeout = utils::TimeSpan::FromSeconds(5);

				auto packetIoPairs = packetIoPickers.pickMultiple(numPeers, timeout, ionet::NodeRoles::Peer);
				if (packetIoPairs.empty()) {
					CATAPULT_LOG(warning) << "could not find any peer for detecting committee stage";
					return thread::make_ready_future(std::vector<CommitteeStage>());
				}

				for (const auto& packetIoPair : packetIoPairs) {
					auto pPacketIo = packetIoPair.io();
					api::RemoteRequestDispatcher dispatcher{*pPacketIo};
					committeeStageFutures.push_back(dispatcher.dispatch(CommitteeStageTraits{}).then([pPacketIo](auto&& stageFuture) {
						return stageFuture.get();
					}));
				}

				return thread::when_all(std::move(committeeStageFutures)).then([](auto&& completedFutures) {
					return thread::get_all_ignore_exceptional(completedFutures.get());
				});
			};
		}

		auto CreateRemoteProposedBlockRetriever(
				const net::PacketIoPickerContainer& packetIoPickers,
				const model::TransactionRegistry& transactionRegistry) {
			return [&packetIoPickers, &transactionRegistry](auto numPeers) {
				std::vector<thread::future<std::shared_ptr<model::Block>>> blockFutures;
				auto timeout = utils::TimeSpan::FromSeconds(5);

				auto packetIoPairs = packetIoPickers.pickMultiple(numPeers, timeout, ionet::NodeRoles::Peer);
				if (packetIoPairs.empty()) {
					CATAPULT_LOG(warning) << "could not find any peer for pulling proposed block";
					return thread::make_ready_future(std::vector<std::shared_ptr<model::Block>>());
				}

				for (const auto& packetIoPair : packetIoPairs) {
					auto pPacketIo = packetIoPair.io();
					api::RemoteRequestDispatcher dispatcher{*pPacketIo};
					blockFutures.push_back(dispatcher.dispatch(PullProposedBlockTraits{transactionRegistry}).then([pPacketIo](auto&& blockFuture) {
						return blockFuture.get();
					}));
				}

				return thread::when_all(std::move(blockFutures)).then([](auto&& completedFutures) {
					return thread::get_all_ignore_exceptional(completedFutures.get());
				});
			};
		}

		auto CreateHarvesterBlockGenerator(extensions::ServiceState& state) {
			auto strategy = state.config().Node.TransactionSelectionStrategy;
			auto executionConfig = extensions::CreateExecutionConfiguration(state.pluginManager());
			harvesting::HarvestingUtFacadeFactory utFacadeFactory(state.cache(), executionConfig);

			return harvesting::CreateHarvesterBlockGenerator(strategy, utFacadeFactory, state.utCache());
		}

		class WeightedVotingServiceRegistrar : public extensions::ServiceRegistrar {
		public:
			explicit WeightedVotingServiceRegistrar(const harvesting::HarvestingConfiguration& config) : m_harvestingConfig(config)
			{}

			extensions::ServiceRegistrarInfo info() const override {
				return { "WeightedVoting", extensions::ServiceRegistrarPhase::Post_Extended_Range_Consumers };
			}

			void registerServiceCounters(extensions::ServiceLocator&) override {
				// no additional counters
			}

			void registerServices(extensions::ServiceLocator&, extensions::ServiceState& state) override {
				auto pValidatorPool = state.pool().pushIsolatedPool("proposal validator");

				auto pServiceGroup = state.pool().pushServiceGroup("weighted voting");
				auto pFsm = pServiceGroup->pushService([](std::shared_ptr<thread::IoThreadPool> pPool) {
					return std::make_shared<WeightedVotingFsm>(pPool);
				});

				auto& packetHandlers = state.packetHandlers();
				const auto& pluginManager = state.pluginManager();
				const auto& pConfigHolder = pluginManager.configHolder();
				const auto& packetIoPickers = state.packetIoPickers();
				const auto& packetPayloadSink = state.hooks().packetPayloadSink();
				auto timeSupplier = state.timeSupplier();
				auto lastBlockElementSupplier = [&storage = state.storage()]() {
					auto storageView = storage.view();
					return storageView.loadBlockElement(storageView.chainHeight());
				};

				RegisterPullCommitteeStageHandler(pFsm, packetHandlers);
				RegisterPushProposedBlockHandler(pFsm, packetHandlers, pluginManager);
				RegisterPullProposedBlockHandler(pFsm, packetHandlers);
				RegisterPushPrevoteMessageHandler(pFsm, packetHandlers);
				RegisterPushPrecommitMessageHandler(pFsm, packetHandlers);

				auto& committeeData = pFsm->committeeData();
				committeeData.setUnlockedAccounts(CreateUnlockedAccounts(m_harvestingConfig));
				committeeData.setBeneficiary(crypto::ParseKey(m_harvestingConfig.Beneficiary));
				auto& actions = pFsm->actions();

				actions.CheckLocalChain = CreateDefaultCheckLocalChainAction(
					pFsm,
					state.hooks().remoteChainHeightsRetriever(),
					pConfigHolder,
					[&state] { return state.storage().view().chainHeight(); });
				actions.ResetLocalChain = CreateDefaultResetLocalChainAction();
				actions.SelectPeers = CreateDefaultSelectPeersAction(
					pFsm,
					CreateRemoteBlockHashesIoRetriever(packetIoPickers),
					pConfigHolder);
				actions.DownloadBlocks = CreateDefaultDownloadBlocksAction(pFsm, state);
				actions.DetectStage = CreateDefaultDetectStageAction(
					pFsm,
					CreateRemoteCommitteeStagesRetriever(packetIoPickers),
					pConfigHolder,
					timeSupplier,
					lastBlockElementSupplier);
				actions.SelectCommittee = CreateDefaultSelectCommitteeAction(
					pFsm,
					pluginManager.getCommitteeManager(),
					pConfigHolder);
				actions.ProposeBlock = CreateDefaultProposeBlockAction(
					pFsm,
					state.cache(),
					pConfigHolder,
					CreateHarvesterBlockGenerator(state),
					lastBlockElementSupplier,
					packetPayloadSink);
				actions.WaitForProposal = CreateDefaultWaitForProposalAction(
					pFsm,
					CreateRemoteProposedBlockRetriever(packetIoPickers, pluginManager.transactionRegistry()),
					pConfigHolder);
				actions.ValidateProposal = CreateDefaultValidateProposalAction(
					pFsm,
					state,
					lastBlockElementSupplier,
					pValidatorPool);
				actions.WaitForPrevotes = CreateDefaultWaitForPrevotesAction(pFsm, pluginManager);
				actions.WaitForPrecommits = CreateDefaultWaitForPrecommitsAction(pFsm, pluginManager);
				actions.BroadcastPrevote = CreateDefaultBroadcastPrevoteAction(pFsm, packetPayloadSink);
				actions.BroadcastPrecommit = CreateDefaultBroadcastPrecommitAction(pFsm, packetPayloadSink);
				actions.CommitConfirmedBlock = CreateDefaultCommitConfirmedBlockAction(
					pFsm,
					state.hooks().completionAwareBlockRangeConsumerFactory()(disruptor::InputSource::Remote_Push));
				actions.IncrementRound = CreateDefaultIncrementRoundAction(pFsm, pConfigHolder);
				actions.ResetRound = CreateDefaultResetRoundAction(pFsm, pConfigHolder);
				actions.WaitForRoundEnd = CreateDefaultWaitForRoundEndAction(pFsm);

				pFsm->start();
			}

		private:
			harvesting::HarvestingConfiguration m_harvestingConfig;
		};
	}

	DECLARE_SERVICE_REGISTRAR(WeightedVoting)(const harvesting::HarvestingConfiguration& config) {
		return std::make_unique<WeightedVotingServiceRegistrar>(config);
	}
}}
