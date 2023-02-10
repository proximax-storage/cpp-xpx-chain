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
			explicit WeightedVotingServiceRegistrar(const harvesting::HarvestingConfiguration& harvestingConfig, const dbrb::DbrbConfiguration& dbrbConfig, std::string resourcesPath)
				: m_harvestingConfig(std::move(harvestingConfig))
				, m_dbrbConfig(std::move(dbrbConfig))
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
				auto pServiceGroup = state.pool().pushServiceGroup("weighted voting");

				auto connectionSettings = extensions::GetConnectionSettings(config);
				auto pWriters = pServiceGroup->pushService(net::CreatePacketWriters, locator.keyPair(), connectionSettings, state);
				locator.registerService(Writers_Service_Name, pWriters);


				auto pFsmShared = pServiceGroup->pushService([pWriters, &config, &keyPair = locator.keyPair(), &state, &dbrbConfig = m_dbrbConfig](const std::shared_ptr<thread::IoThreadPool>& pPool) {
					dbrb::TransactionSender transactionSender(keyPair, config.Immutable, dbrbConfig, state.hooks().transactionRangeConsumerFactory()(disruptor::InputSource::Local));
					auto pDbrbProcess = std::make_shared<dbrb::DbrbProcess>(pWriters, state.packetIoPickers(), config::ToLocalDbrbNode(config), keyPair, pPool, std::move(transactionSender));
					return std::make_shared<WeightedVotingFsm>(pPool, config, pDbrbProcess, state.pluginManager().dbrbViewFetcher());
				});

				const auto& pluginManager = state.pluginManager();
				auto pPacketHandlers = std::make_shared<ionet::ServerPacketHandlers>(config.Node.MaxPacketDataSize.bytes32());
				pFsmShared->dbrbProcess()->registerPacketHandlers(*pPacketHandlers);
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

				RegisterPullConfirmedBlockHandler(pFsmShared, state.packetHandlers());
				RegisterPullRemoteNodeStateHandler(pFsmShared, state.packetHandlers(), pConfigHolder, blockElementGetter, lastBlockElementSupplier);
				dbrb::RegisterPushNodesHandler(pFsmShared->dbrbProcess(), config.Immutable.NetworkIdentifier, state.packetHandlers());
				dbrb::RegisterPullNodesHandler(pFsmShared->dbrbProcess(), state.packetHandlers());

				auto pReaders = pServiceGroup->pushService(net::CreatePacketReaders, *pPacketHandlers, locator.keyPair(), connectionSettings, 2u);
				extensions::BootServer(*pServiceGroup, config.Node.DbrbPort, Service_Id, config, [&acceptor = *pReaders](
					const auto& socketInfo,
					const auto& callback) {
					acceptor.accept(socketInfo, callback);
				});
				locator.registerService(Readers_Service_Name, pReaders);

				auto& committeeData = pFsmShared->committeeData();
				committeeData.setUnlockedAccounts(CreateUnlockedAccounts(m_harvestingConfig));
				committeeData.setBeneficiary(crypto::ParseKey(m_harvestingConfig.Beneficiary));
				auto& actions = pFsmShared->actions();

				actions.CheckLocalChain = CreateDefaultCheckLocalChainAction(
					pFsmShared,
					CreateRemoteNodeStateRetriever(pConfigHolder, lastBlockElementSupplier, state.packetIoPickers()),
					pConfigHolder,
					lastBlockElementSupplier,
					importanceGetter,
					pluginManager.getCommitteeManager(),
					m_dbrbConfig);
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
				actions.ValidateProposal = CreateDefaultValidateProposalAction(
					pFsmShared,
					state,
					lastBlockElementSupplier,
					pValidatorPool);
				actions.WaitForProposal = CreateDefaultWaitForProposalAction(pFsmShared);
				actions.WaitForPrevotePhaseEnd = CreateDefaultWaitForPrevotePhaseEndAction(pFsmShared, pluginManager.getCommitteeManager(), pConfigHolder);
				actions.AddPrevote = CreateDefaultAddPrevoteAction(pFsmShared);
				actions.AddPrecommit = CreateDefaultAddPrecommitAction(pFsmShared);
				actions.WaitForPrecommitPhaseEnd = CreateDefaultWaitForPrecommitPhaseEndAction(pFsmShared, pluginManager.getCommitteeManager(), pConfigHolder);
				actions.UpdateConfirmedBlock = CreateDefaultUpdateConfirmedBlockAction(pFsmShared, pluginManager.getCommitteeManager());
				actions.RequestConfirmedBlock = CreateDefaultRequestConfirmedBlockAction(pFsmShared, state, lastBlockElementSupplier);
				actions.CommitConfirmedBlock = CreateDefaultCommitConfirmedBlockAction(
					pFsmShared,
					blockRangeConsumer,
					pConfigHolder,
					pluginManager.getCommitteeManager(),
					m_dbrbConfig);
				actions.IncrementRound = CreateDefaultIncrementRoundAction(pFsmShared, pConfigHolder);
				actions.ResetRound = CreateDefaultResetRoundAction(pFsmShared, pConfigHolder, pluginManager.getCommitteeManager());

				pFsmShared->start();
			}

		private:
			harvesting::HarvestingConfiguration m_harvestingConfig;
			dbrb::DbrbConfiguration m_dbrbConfig;
		};
	}

	DECLARE_SERVICE_REGISTRAR(WeightedVoting)(const harvesting::HarvestingConfiguration& harvestingConfig, const dbrb::DbrbConfiguration& dbrbConfig, std::string resourcesPath) {
		return std::make_unique<WeightedVotingServiceRegistrar>(harvestingConfig, dbrbConfig, std::move(resourcesPath));
	}
}}
