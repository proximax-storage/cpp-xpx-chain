/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "WeightedVotingService.h"
#include "WeightedVotingFsm.h"
#include "dbrb/DbrbPacketHandlers.h"
#include "catapult/api/RemoteRequestDispatcher.h"
#include "catapult/cache_core/ImportanceView.h"
#include "catapult/crypto/KeyUtils.h"
#include "catapult/extensions/ExecutionConfigurationFactory.h"
#include "catapult/extensions/NetworkUtils.h"
#include "catapult/extensions/ServiceLocator.h"
#include "catapult/harvesting_core/HarvestingUtFacadeFactory.h"
#include "catapult/harvesting_core/UnlockedAccounts.h"
#include "catapult/io/BlockStorageCache.h"
#include "catapult/ionet/NodeContainer.h"
#include "catapult/net/PacketReaders.h"
#include "catapult/net/PacketWriters.h"

namespace catapult { namespace fastfinality {

	namespace {
		constexpr auto Writers_Service_Name = "weightedvoting.writers";
		constexpr auto Readers_Service_Name = "weightedvoting.readers";
		constexpr auto Fsm_Service_Name = "weightedvoting.fsm";
		constexpr auto Service_Id = ionet::ServiceIdentifier(0x54654144);

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
				const std::weak_ptr<WeightedVotingFsm>& pFsmWeak,
				const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder,
				const model::BlockElementSupplier& lastBlockElementSupplier) {
			return [pFsmWeak, pConfigHolder, lastBlockElementSupplier]() {
				auto pFsmShared = pFsmWeak.lock();
				if (!pFsmShared || pFsmShared->stopped() || pFsmShared->dbrbProcess()->currentView().Data.empty())
					return std::vector<RemoteNodeState>();

				auto pPromise = std::make_shared<thread::promise<std::vector<RemoteNodeState>>>();

				boost::asio::post(pFsmShared->dbrbProcess()->strand(), [pFsmWeak, pConfigHolder, lastBlockElementSupplier, pPromise]() {
					auto pFsmShared = pFsmWeak.lock();
					if (!pFsmShared || pFsmShared->stopped()) {
						pPromise->set_value(std::vector<RemoteNodeState>());
						return;
					}

					std::vector<thread::future<RemoteNodeState>> remoteNodeStateFutures;

					auto chainHeight = lastBlockElementSupplier()->Block.Height;
					const auto& config = pConfigHolder->Config(chainHeight);
					const auto maxBlocksPerSyncAttempt = config.Node.MaxBlocksPerSyncAttempt;
					const auto targetHeight = chainHeight + Height(maxBlocksPerSyncAttempt);

					auto dbrbBootstrapProcesses = config.Network.DbrbBootstrapProcesses;
					if (dbrbBootstrapProcesses.empty())
						dbrbBootstrapProcesses = pConfigHolder->Config(chainHeight + Height(1)).Network.DbrbBootstrapProcesses;
					auto minOpinionNumber = dbrb::View{dbrbBootstrapProcesses}.quorumSize() - 1;

					const auto& view = pFsmShared->dbrbProcess()->currentView();
					auto& packetIoPairMap = pFsmShared->dbrbProcess()->messageSender().getPacketIos(view.Data);
					CATAPULT_LOG(debug) << "found " << packetIoPairMap.size() << " peer(s) for pulling remote node states, min opinion number " << minOpinionNumber;
					if (packetIoPairMap.size() < minOpinionNumber) {
						pPromise->set_value(std::vector<RemoteNodeState>());
						return;
					}

					for (const auto& pair : packetIoPairMap) {
						auto pPacketIoPair = pair.second;
						api::RemoteRequestDispatcher dispatcher{*pPacketIoPair->io()};
						remoteNodeStateFutures.push_back(dispatcher.dispatch(RemoteNodeStateTraits{}, targetHeight).then([pPacketIoPair](auto&& stateFuture) {
							auto remoteNodeState = stateFuture.get();
							remoteNodeState.NodeKey = pPacketIoPair->node().identityKey();
							return remoteNodeState;
						}));
					}

					auto nodeStates = thread::when_all(std::move(remoteNodeStateFutures)).then([](auto&& completedFutures) {
						return thread::get_all_ignore_exceptional(completedFutures.get());
					}).get();
					CATAPULT_LOG(debug) << "retrieved " << nodeStates.size() << " node states, min opinion number " << minOpinionNumber;

					if (nodeStates.empty()) {
						packetIoPairMap.clear();
					} else {
						for (auto iter = packetIoPairMap.begin(); iter != packetIoPairMap.end();) {
							if (std::find_if(nodeStates.begin(), nodeStates.end(), [&iter](const auto& nodeState) { return iter->first == nodeState.NodeKey; }) == nodeStates.end()) {
								iter = packetIoPairMap.erase(iter);
							} else {
								iter++;
							}
						}
					}

					if (nodeStates.size() < minOpinionNumber)
						nodeStates.clear();

					pPromise->set_value(std::move(nodeStates));
				});

				return pPromise->get_future().get();
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
			explicit WeightedVotingServiceRegistrar(
					const harvesting::HarvestingConfiguration& harvestingConfig,
					const dbrb::DbrbConfiguration& dbrbConfig,
					std::shared_ptr<dbrb::TransactionSender> pTransactionSender)
				: m_harvestingConfig(harvestingConfig)
				, m_dbrbConfig(dbrbConfig)
				, m_pTransactionSender(std::move(pTransactionSender))
			{}

			extensions::ServiceRegistrarInfo info() const override {
				return { "WeightedVoting", extensions::ServiceRegistrarPhase::Post_Extended_Range_Consumers };
			}

			void registerServiceCounters(extensions::ServiceLocator& locator) override {
				locator.registerServiceCounter<net::PacketWriters>(Writers_Service_Name, "WV WRITERS", [](const auto& writers) {
					return writers.numActiveWriters();
				});
				locator.registerServiceCounter<net::PacketReaders>(Readers_Service_Name, "WV READERS", [](const auto& readers) {
					return readers.numActiveReaders();
				});
			}

			void registerServices(extensions::ServiceLocator& locator, extensions::ServiceState& state) override {
				const auto& config = state.config();
				const auto& nextConfig = state.config(state.storage().view().chainHeight() + Height(1));
				bool weightedVotingEnabled = config.Network.EnableWeightedVoting || nextConfig.Network.EnableWeightedVoting;
				if (!weightedVotingEnabled)
					CATAPULT_THROW_RUNTIME_ERROR("weighted voting is not enabled")

				auto pValidatorPool = state.pool().pushIsolatedPool("proposal validator");
				auto pDbrbPool = state.pool().pushIsolatedPool("dbrb");
				auto pWeightedVotingFsmPool = state.pool().pushIsolatedPool("weighted voting fsm");
				auto pServiceGroup = state.pool().pushServiceGroup("weighted voting");

				auto connectionSettings = extensions::GetConnectionSettings(config);
				auto pWriters = pServiceGroup->pushService(net::CreatePacketWriters, locator.keyPair(), connectionSettings, state);
				locator.registerService(Writers_Service_Name, pWriters);

				auto pUnlockedAccounts = CreateUnlockedAccounts(m_harvestingConfig);
				auto pFsmShared = pServiceGroup->pushService([
						pWritersWeak = std::weak_ptr<net::PacketWriters>(pWriters),
						&config,
						&keyPair = locator.keyPair(),
						&state,
						&dbrbConfig = m_dbrbConfig,
						pUnlockedAccounts,
						pTransactionSender = m_pTransactionSender,
						pDbrbPool,
						pWeightedVotingFsmPool](const std::shared_ptr<thread::IoThreadPool>&) {
					pTransactionSender->init(&keyPair, config.Immutable, dbrbConfig, state.hooks().transactionRangeConsumerFactory()(disruptor::InputSource::Local), pUnlockedAccounts);
					auto pDbrbProcess = std::make_shared<dbrb::DbrbProcess>(pWritersWeak, state.packetIoPickers(), config::ToLocalDbrbNode(config),
						keyPair, pDbrbPool, pTransactionSender, state.pluginManager().dbrbViewFetcher());
					return std::make_shared<WeightedVotingFsm>(pWeightedVotingFsmPool, config, pDbrbProcess);
				});
				locator.registerService(Fsm_Service_Name, pFsmShared);

				const auto& pluginManager = state.pluginManager();
				pFsmShared->dbrbProcess()->registerPacketHandlers(pFsmShared->packetHandlers());
				std::weak_ptr<WeightedVotingFsm> pFsmWeak = pFsmShared;
				pFsmShared->dbrbProcess()->setDeliverCallback([pFsmWeak, &pluginManager](const std::shared_ptr<ionet::Packet>& pPacket) {
					TRY_GET_FSM()

					switch (pPacket->Type) {
						case ionet::PacketType::Push_Proposed_Block: {
							PushProposedBlock(pFsmShared, pluginManager, *pPacket);
							break;
						}
						case ionet::PacketType::Push_Prevote_Messages: {
							PushPrevoteMessages(pFsmShared, *pPacket);
							break;
						}
						case ionet::PacketType::Push_Precommit_Messages: {
							PushPrecommitMessages(pFsmShared, *pPacket);
							break;
						}
					}
				});

				const auto& pConfigHolder = pluginManager.configHolder();
				const auto& storage = state.storage();
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

				RegisterPullConfirmedBlockHandler(pFsmShared, state.packetHandlers());
				RegisterPullRemoteNodeStateHandler(pFsmShared, pFsmShared->packetHandlers(), locator.keyPair().publicKey(), blockElementGetter, lastBlockElementSupplier);
				dbrb::RegisterPushNodesHandler(pFsmShared->dbrbProcess(), config.Immutable.NetworkIdentifier, state.packetHandlers());
				dbrb::RegisterPullNodesHandler(pFsmShared->dbrbProcess(), state.packetHandlers());

				auto pReaders = pServiceGroup->pushService(net::CreatePacketReaders, pFsmShared->packetHandlers(), locator.keyPair(), connectionSettings, 2u);
				extensions::BootServer(*pServiceGroup, config.Node.DbrbPort, Service_Id, config, [&acceptor = *pReaders](
					const auto& socketInfo,
					const auto& callback) {
					acceptor.accept(socketInfo, callback);
				});
				locator.registerService(Readers_Service_Name, pReaders);

				auto& committeeData = pFsmShared->committeeData();
				committeeData.setUnlockedAccounts(pUnlockedAccounts);
				committeeData.setBeneficiary(crypto::ParseKey(m_harvestingConfig.Beneficiary));
				auto& actions = pFsmShared->actions();

				actions.CheckLocalChain = CreateDefaultCheckLocalChainAction(
					pFsmShared,
					CreateRemoteNodeStateRetriever(pFsmShared, pConfigHolder, lastBlockElementSupplier),
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
					state.timeSupplier(),
					lastBlockElementSupplier,
					pluginManager.getCommitteeManager());
				actions.SelectCommittee = CreateDefaultSelectCommitteeAction(
					pFsmShared,
					pluginManager.getCommitteeManager(),
					pConfigHolder);
				actions.ProposeBlock = CreateDefaultProposeBlockAction(
					pFsmShared,
					state.cache(),
					pConfigHolder,
					CreateHarvesterBlockGenerator(state),
					lastBlockElementSupplier);
				actions.ValidateProposal = CreateDefaultValidateProposalAction(pFsmShared, state, lastBlockElementSupplier, pValidatorPool);
				actions.WaitForProposal = CreateDefaultWaitForProposalAction(pFsmShared);
				actions.WaitForPrevotePhaseEnd = CreateDefaultWaitForPrevotePhaseEndAction(pFsmShared, pluginManager.getCommitteeManager(), pConfigHolder);
				actions.AddPrevote = CreateDefaultAddPrevoteAction(pFsmShared);
				actions.AddPrecommit = CreateDefaultAddPrecommitAction(pFsmShared);
				actions.WaitForPrecommitPhaseEnd = CreateDefaultWaitForPrecommitPhaseEndAction(pFsmShared, pluginManager.getCommitteeManager(), pConfigHolder);
				actions.UpdateConfirmedBlock = CreateDefaultUpdateConfirmedBlockAction(pFsmShared, pluginManager.getCommitteeManager());
				actions.RequestConfirmedBlock = CreateDefaultRequestConfirmedBlockAction(pFsmShared, state, lastBlockElementSupplier);
				actions.CommitConfirmedBlock = CreateDefaultCommitConfirmedBlockAction(pFsmShared, blockRangeConsumer, state);
				actions.IncrementRound = CreateDefaultIncrementRoundAction(pFsmShared, pConfigHolder);
				actions.ResetRound = CreateDefaultResetRoundAction(pFsmShared, pConfigHolder, pluginManager.getCommitteeManager());

				m_pTransactionSender.reset();
				pFsmShared->start();
			}

		private:
			harvesting::HarvestingConfiguration m_harvestingConfig;
			dbrb::DbrbConfiguration m_dbrbConfig;
			std::shared_ptr<dbrb::TransactionSender> m_pTransactionSender;

		};
	}

	DECLARE_SERVICE_REGISTRAR(WeightedVoting)(
			const harvesting::HarvestingConfiguration& harvestingConfig,
			const dbrb::DbrbConfiguration& dbrbConfig,
			std::shared_ptr<dbrb::TransactionSender> pTransactionSender) {
		return std::make_unique<WeightedVotingServiceRegistrar>(harvestingConfig, dbrbConfig, std::move(pTransactionSender));
	}
}}
