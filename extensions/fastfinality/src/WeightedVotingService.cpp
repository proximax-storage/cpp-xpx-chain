/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "WeightedVotingService.h"
#include "WeightedVotingFsm.h"
#include "catapult/api/RemoteChainApi.h"
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

		auto CreateRemoteChainHeightsRetriever(const net::PacketIoPickerContainer& packetIoPickers) {
			return [&packetIoPickers]() {
				std::vector<thread::future<Height>> heightFutures;
				auto timeout = utils::TimeSpan::FromSeconds(5);

				auto packetIoPairs = packetIoPickers.pickMultiple(timeout);
				CATAPULT_LOG(debug) << "found " << packetIoPairs.size() << " peer(s) for detecting chain heights";
				if (packetIoPairs.empty())
					return thread::make_ready_future(std::vector<Height>());

				for (const auto& packetIoPair : packetIoPairs) {
					auto pPacketIo = packetIoPair.io();
					auto pChainApi = api::CreateRemoteChainApiWithoutRegistry(*pPacketIo);
					heightFutures.push_back(pChainApi->chainInfo().then([pPacketIo](auto&& infoFuture) {
						return infoFuture.get().Height;
					}));
				}

				return thread::when_all(std::move(heightFutures)).then([](auto&& completedFutures) {
					return thread::get_all_ignore_exceptional(completedFutures.get());
				});
			};
		}

		auto CreateRemoteBlockHashesIoRetriever(const net::PacketIoPickerContainer& packetIoPickers) {
			return [&packetIoPickers](const Height& height) {
				std::vector<thread::future<std::pair<Hash256, Key>>> futures;
				auto timeout = utils::TimeSpan::FromSeconds(5);

				auto packetIoPairs = packetIoPickers.pickMultiple(timeout);
				CATAPULT_LOG(debug) << "found " << packetIoPairs.size() << " peer(s) for pulling block hashes";
				if (packetIoPairs.empty())
					return thread::make_ready_future(std::vector<std::pair<Hash256, Key>>());

				for (const auto& packetIoPair : packetIoPairs) {
					auto pPacketIoPair = std::make_shared<ionet::NodePacketIoPair>(packetIoPair);
					auto pChainApi = api::CreateRemoteChainApiWithoutRegistry(*pPacketIoPair->io());
					futures.push_back(pChainApi->hashesFrom(height, 1u).then([pPacketIoPair](auto&& hashesFuture) {
						auto hash = *hashesFuture.get().cbegin();
						return std::make_pair(hash, pPacketIoPair->node().identityKey());
					}));
				}

				return thread::when_all(std::move(futures)).then([](auto&& completedFutures) {
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
				auto timeSupplier = state.timeSupplier();
				auto blockRangeConsumer = state.hooks().completionAwareBlockRangeConsumerFactory()(disruptor::InputSource::Remote_Pull);
				auto lastBlockElementSupplier = [&storage = state.storage()]() {
					auto storageView = storage.view();
					return storageView.loadBlockElement(storageView.chainHeight());
				};
				pluginManager.getCommitteeManager().setLastBlockElementSupplier(lastBlockElementSupplier);

				RegisterPushProposedBlockHandler(pFsmShared, packetHandlers, pluginManager);
				RegisterPushConfirmedBlockHandler(pFsmShared, packetHandlers, pluginManager);
				RegisterPushPrevoteMessageHandler(pFsmShared, packetHandlers);
				RegisterPushPrecommitMessageHandler(pFsmShared, packetHandlers);

				auto& committeeData = pFsmShared->committeeData();
				committeeData.setUnlockedAccounts(CreateUnlockedAccounts(m_harvestingConfig));
				committeeData.setBeneficiary(crypto::ParseKey(m_harvestingConfig.Beneficiary));
				auto& actions = pFsmShared->actions();

				actions.CheckLocalChain = CreateDefaultCheckLocalChainAction(
					pFsmShared,
					CreateRemoteChainHeightsRetriever(packetIoPickers),
					pConfigHolder,
					[&state] { return state.storage().view().chainHeight(); });
				actions.ResetLocalChain = CreateDefaultResetLocalChainAction();
				actions.SelectPeers = CreateDefaultSelectPeersAction(
					pFsmShared,
					CreateRemoteBlockHashesIoRetriever(packetIoPickers),
					pConfigHolder);
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
