/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "WeightedVotingService.h"
#include "WeightedVotingFsm.h"
#include <catapult/api/RemoteRequestDispatcher.h>
#include "catapult/cache_core/ImportanceView.h"
#include "catapult/crypto/KeyUtils.h"
#include "catapult/extensions/ExecutionConfigurationFactory.h"
#include "catapult/extensions/ServiceLocator.h"
#include "catapult/harvesting_core/HarvestingUtFacadeFactory.h"
#include "catapult/harvesting_core/UnlockedAccounts.h"
#include "catapult/io/BlockStorageCache.h"
#include "catapult/thread/MultiServicePool.h"

namespace catapult { namespace fastfinality {

	namespace {
		constexpr auto Service_Name = "weightedvoting";

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

		auto CreateRemoteNodeStateRetriever(
				const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder,
				const model::BlockElementSupplier& lastBlockElementSupplier,
				const net::PacketIoPickerContainer& packetIoPickers) {
			return [pConfigHolder, lastBlockElementSupplier, &packetIoPickers]() {
				std::vector<thread::future<RemoteNodeState>> remoteNodeStateFutures;
				auto timeout = utils::TimeSpan::FromSeconds(5);

				auto packetIoPairs = packetIoPickers.pickMultiple(timeout);
				CATAPULT_LOG(debug) << "found " << packetIoPairs.size() << " peer(s) for pulling remote node states";
				if (packetIoPairs.empty())
					return thread::make_ready_future(std::vector<RemoteNodeState>());

				const auto maxBlocksPerSyncAttempt = pConfigHolder->Config().Node.MaxBlocksPerSyncAttempt;
				const auto targetHeight = lastBlockElementSupplier()->Block.Height + Height(maxBlocksPerSyncAttempt);

				for (const auto& packetIoPair : packetIoPairs) {
					auto pPacketIoPair = std::make_shared<ionet::NodePacketIoPair>(packetIoPair);
					api::RemoteRequestDispatcher dispatcher{*pPacketIoPair->io()};
					remoteNodeStateFutures.push_back(dispatcher.dispatch(RemoteNodeStateTraits{}, targetHeight).then([pPacketIoPair](auto&& stateFuture) {
						auto remoteNodeState = stateFuture.get();
						remoteNodeState.NodeKey = pPacketIoPair->node().identityKey();
						return remoteNodeState;
					}));
				}

				return thread::when_all(std::move(remoteNodeStateFutures)).then([](auto&& completedFutures) {
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

			void registerServices(extensions::ServiceLocator& locator, extensions::ServiceState& state) override {
				if (!state.config().Network.EnableWeightedVoting)
					CATAPULT_THROW_RUNTIME_ERROR("weighted voting is not enabled");

				auto pValidatorPool = state.pool().pushIsolatedPool("proposal validator");

				auto pServiceGroup = state.pool().pushServiceGroup("weighted voting");
				auto pFsmShared = pServiceGroup->pushService([](std::shared_ptr<thread::IoThreadPool> pPool) {
					return std::make_shared<WeightedVotingFsm>(pPool);
				});
				locator.registerService(Service_Name, pFsmShared);

				auto& packetHandlers = state.packetHandlers();
				const auto& pluginManager = state.pluginManager();
				const auto& pConfigHolder = pluginManager.configHolder();
				const auto& packetIoPickers = state.packetIoPickers();
				const auto& packetPayloadSink = state.hooks().packetPayloadSink();
				const auto& storage = state.storage();
				auto timeSupplier = state.timeSupplier();
				auto blockRangeConsumer = state.hooks().completionAwareBlockRangeConsumerFactory()(disruptor::InputSource::Remote_Pull);
				auto lastBlockElementSupplier = [&storage]() {
					auto storageView = storage.view();
					return storageView.loadBlockElement(storageView.chainHeight());
				};
				auto blockElementGetter = [&storage](const Height& height) {
				  return storage.view().loadBlockElement(height);
				};
				auto importanceGetter = [&state](const Key& identityKey) {
					auto height = state.cache().height();
					const auto& cache = state.cache().sub<cache::AccountStateCache>();
					auto view = cache.createView(height);
					cache::ImportanceView importanceView(view->asReadOnly());
					return importanceView.getAccountImportanceOrDefault(identityKey, height).unwrap();
				};
				pluginManager.getCommitteeManager().setLastBlockElementSupplier(lastBlockElementSupplier);

				RegisterPushProposedBlockHandler(pFsmShared, packetHandlers, pluginManager);
				RegisterPushConfirmedBlockHandler(pFsmShared, packetHandlers, pluginManager);
				RegisterPushPrevoteMessageHandler(pFsmShared, packetHandlers);
				RegisterPushPrecommitMessageHandler(pFsmShared, packetHandlers);
				// TODO: Consider rewriting to (pFsmShared, packetHandlers, pluginManager, storage)
				RegisterPullRemoteNodeStateHandler(pFsmShared, packetHandlers, pConfigHolder, blockElementGetter, lastBlockElementSupplier);

				auto& committeeData = pFsmShared->committeeData();
				committeeData.setUnlockedAccounts(CreateUnlockedAccounts(m_harvestingConfig));
				committeeData.setBeneficiary(crypto::ParseKey(m_harvestingConfig.Beneficiary));
				auto& actions = pFsmShared->actions();

				actions.CheckLocalChain = CreateDefaultCheckLocalChainAction(
					pFsmShared,
					CreateRemoteNodeStateRetriever(pConfigHolder, lastBlockElementSupplier, packetIoPickers),
					pConfigHolder,
					lastBlockElementSupplier,
					importanceGetter,
					pluginManager.getCommitteeManager());
				actions.ResetLocalChain = CreateDefaultResetLocalChainAction();
				actions.DownloadBlocks = CreateDefaultDownloadBlocksAction(
					pFsmShared,
					state,
					blockRangeConsumer,
					pluginManager.getCommitteeManager());
				actions.DetectStage = CreateDefaultDetectStageAction(
					pFsmShared,
					pConfigHolder,
					timeSupplier,
					lastBlockElementSupplier,
					pluginManager.getCommitteeManager());
				actions.SelectCommittee = CreateDefaultSelectCommitteeAction(
					pFsmShared,
					pluginManager.getCommitteeManager(),
					pConfigHolder,
					timeSupplier);
				actions.ProposeBlock = CreateDefaultProposeBlockAction(
					pFsmShared,
					state.cache(),
					pConfigHolder,
					CreateHarvesterBlockGenerator(state),
					lastBlockElementSupplier);
				actions.WaitForProposal = CreateDefaultWaitForProposalAction(pFsmShared);
				actions.ValidateProposal = CreateDefaultValidateProposalAction(
					pFsmShared,
					state,
					lastBlockElementSupplier,
					pValidatorPool);
				actions.WaitForProposalPhaseEnd = CreateDefaultWaitForProposalPhaseEndAction(pFsmShared, pConfigHolder, packetPayloadSink);
				actions.WaitForPrevotes = CreateDefaultWaitForPrevotesAction(pFsmShared, pluginManager, packetPayloadSink);
				actions.WaitForPrecommits = CreateDefaultWaitForPrecommitsAction(pFsmShared, pluginManager, packetPayloadSink);
				actions.AddPrevote = CreateDefaultAddPrevoteAction(pFsmShared);
				actions.AddPrecommit = CreateDefaultAddPrecommitAction(pFsmShared);
				actions.UpdateConfirmedBlock = CreateDefaultUpdateConfirmedBlockAction(pFsmShared, pluginManager.getCommitteeManager());
				actions.CommitConfirmedBlock = CreateDefaultCommitConfirmedBlockAction(
					pFsmShared,
					blockRangeConsumer,
					pConfigHolder,
					packetPayloadSink,
					pluginManager.getCommitteeManager());
				actions.IncrementRound = CreateDefaultIncrementRoundAction(pFsmShared, pConfigHolder);
				actions.ResetRound = CreateDefaultResetRoundAction(pFsmShared, pConfigHolder, pluginManager.getCommitteeManager());
				actions.WaitForConfirmedBlock = CreateDefaultWaitForConfirmedBlockAction(pFsmShared, lastBlockElementSupplier);
			}

		private:
			harvesting::HarvestingConfiguration m_harvestingConfig;
		};

		std::shared_ptr<WeightedVotingFsm> GetWeightedVotingFsm(const extensions::ServiceLocator& locator) {
			return locator.service<WeightedVotingFsm>(Service_Name);
		}

		class WeightedVotingStartServiceRegistrar : public extensions::ServiceRegistrar {
		public:
			extensions::ServiceRegistrarInfo info() const override {
				return { "WeightedVotingStart", extensions::ServiceRegistrarPhase::Post_Packet_Readers };
			}

			void registerServiceCounters(extensions::ServiceLocator&) override {
				// no additional counters
			}

			void registerServices(extensions::ServiceLocator& locator, extensions::ServiceState& state) override {
				auto pFsmShared = GetWeightedVotingFsm(locator);

				pFsmShared->setPeerConnectionTasks(state);
				pFsmShared->start();
			}
		};
	}

	DECLARE_SERVICE_REGISTRAR(WeightedVoting)(const harvesting::HarvestingConfiguration& config) {
		return std::make_unique<WeightedVotingServiceRegistrar>(config);
	}

	DECLARE_SERVICE_REGISTRAR(WeightedVotingStart)() {
		return std::make_unique<WeightedVotingStartServiceRegistrar>();
	}
}}
