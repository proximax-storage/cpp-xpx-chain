/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "FastFinalityService.h"
#include "FastFinalityFsm.h"
#include "FastFinalityHandlers.h"
#include "fastfinality/src/utils/FastFinalityUtils.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "catapult/cache_core/ImportanceView.h"
#include "catapult/crypto/KeyUtils.h"
#include "catapult/extensions/NetworkUtils.h"
#include "catapult/extensions/ServiceLocator.h"
#include "catapult/handlers/ChainHandlers.h"
#include "catapult/io/BlockStorageCache.h"
#include "catapult/ionet/NodeContainer.h"
#include "catapult/net/PacketReaders.h"
#include "catapult/net/PacketWriters.h"
#include <utility>

namespace catapult { namespace fastfinality {

	namespace {
		constexpr auto Writers_Service_Name = "fast.finality.writers";
		constexpr auto Readers_Service_Name = "fast.finality.readers";
		constexpr auto Service_Id = ionet::ServiceIdentifier(0x54654144);

		class FastFinalityServiceRegistrar : public extensions::ServiceRegistrar {
		public:
			explicit FastFinalityServiceRegistrar(
					harvesting::HarvestingConfiguration  harvestingConfig,
					const dbrb::DbrbConfiguration& dbrbConfig,
					std::shared_ptr<dbrb::TransactionSender> pTransactionSender)
				: m_harvestingConfig(std::move(harvestingConfig))
				, m_dbrbConfig(dbrbConfig)
				, m_pTransactionSender(std::move(pTransactionSender))
			{}

			extensions::ServiceRegistrarInfo info() const override {
				return { "FastFinality", extensions::ServiceRegistrarPhase::Post_Extended_Range_Consumers };
			}

			void registerServiceCounters(extensions::ServiceLocator& locator) override {
				locator.registerServiceCounter<net::PacketWriters>(Writers_Service_Name, "FF WRITERS", [](const auto& writers) {
					return writers.numActiveWriters();
				});
				locator.registerServiceCounter<net::PacketReaders>(Readers_Service_Name, "FF READERS", [](const auto& readers) {
					return readers.numActiveReaders();
				});
			}

			void registerServices(extensions::ServiceLocator& locator, extensions::ServiceState& state) override {
				const auto& config = state.config();
				const auto& nextConfig = state.config(state.storage().view().chainHeight() + Height(1)).Network;
				if (!nextConfig.EnableDbrbFastFinality) {
					CATAPULT_LOG(warning) << "DBRB fast finality is not enabled";
					return;
				}

				auto pValidatorPool = state.pool().pushIsolatedPool("proposal validator");
				auto pDbrbPool = state.pool().pushIsolatedPool("dbrb");
				auto pFastFinalityFsmPool = state.pool().pushIsolatedPool("fast finality fsm");
				auto pServiceGroup = state.pool().pushServiceGroup("fast finality");

				auto connectionSettings = extensions::GetConnectionSettings(config);
				auto pWriters = pServiceGroup->pushService(net::CreatePacketWriters, locator.keyPair(), connectionSettings, state);
				locator.registerService(Writers_Service_Name, pWriters);

				auto pUnlockedAccounts = CreateUnlockedAccounts(m_harvestingConfig);
				auto dbrbShardingEnabled = nextConfig.EnableDbrbSharding;
				auto dbrbShardSize = config.Network.DbrbShardSize;
				auto pFsmShared = pServiceGroup->pushService([
						pWritersWeak = std::weak_ptr<net::PacketWriters>(pWriters),
						&config,
						&keyPair = locator.keyPair(),
						&state,
						&dbrbConfig = m_dbrbConfig,
						pUnlockedAccounts,
						pTransactionSender = m_pTransactionSender,
						pDbrbPool,
						pFastFinalityFsmPool,
						dbrbShardingEnabled,
						dbrbShardSize](const std::shared_ptr<thread::IoThreadPool>&) {
					pTransactionSender->init(&keyPair, config.Immutable, dbrbConfig, state.hooks().transactionRangeConsumerFactory()(disruptor::InputSource::Local), pUnlockedAccounts);
					auto pMessageSender = dbrb::CreateMessageSender(config::ToLocalNode(config), pWritersWeak, state.nodes(), dbrbConfig.IsDbrbProcess, pTransactionSender, pDbrbPool, dbrbConfig.ResendMessagesInterval);
					if (dbrbShardingEnabled) {
						auto pDbrbProcess = std::make_shared<dbrb::ShardedDbrbProcess>(keyPair, pMessageSender, pDbrbPool, pTransactionSender, state.pluginManager().dbrbViewFetcher(), dbrbShardSize);
						return std::make_shared<FastFinalityFsm>(pFastFinalityFsmPool, config, pDbrbProcess, state.pluginManager());
					} else {
						auto pDbrbProcess = std::make_shared<dbrb::DbrbProcess>(keyPair, pMessageSender, pDbrbPool, pTransactionSender, state.pluginManager().dbrbViewFetcher());
						return std::make_shared<FastFinalityFsm>(pFastFinalityFsmPool, config, pDbrbProcess, state.pluginManager());
					}
				});

				const auto& pluginManager = state.pluginManager();
				pFsmShared->dbrbProcess().registerPacketHandlers(pFsmShared->packetHandlers());
				std::weak_ptr<FastFinalityFsm> pFsmWeak = pFsmShared;
				pFsmShared->dbrbProcess().setDeliverCallback([pFsmWeak, &pluginManager](const std::shared_ptr<ionet::Packet>& pPacket) {
					TRY_GET_FSM()

					switch (pPacket->Type) {
						case ionet::PacketType::Push_Block: {
							PushBlock(*pFsmShared, pluginManager, *pPacket);
							break;
						}
					}
				});

				const auto& storage = state.storage();
				auto lastBlockElementSupplier = [&storage]() {
					auto storageView = storage.view();
					return storageView.loadBlockElement(storageView.chainHeight());
				};
				pFsmShared->dbrbProcess().setValidationCallback([pFsmWeak, &pluginManager, &state, lastBlockElementSupplier, pValidatorPool](const std::shared_ptr<ionet::Packet>& pPacket) {
					auto pFsmShared = pFsmWeak.lock();
					if (!pFsmShared || pFsmShared->stopped())
						return false;

					switch (pPacket->Type) {
						case ionet::PacketType::Push_Block: {
							return ValidateBlock(*pFsmShared, *pPacket, state, lastBlockElementSupplier, pValidatorPool);
						}
					}

					return false;
				});

				const auto& pConfigHolder = pluginManager.configHolder();
				auto blockRangeConsumer = state.hooks().completionAwareBlockRangeConsumerFactory()(disruptor::InputSource::Remote_Pull);
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
				pluginManager.getCommitteeManager(4).setLastBlockElementSupplier(lastBlockElementSupplier);
				pluginManager.getCommitteeManager(5).setLastBlockElementSupplier(lastBlockElementSupplier);
				pluginManager.getCommitteeManager(6).setLastBlockElementSupplier(lastBlockElementSupplier);

				RegisterPullRemoteNodeStateHandler(pFsmShared, pFsmShared->packetHandlers(), locator.keyPair().publicKey(), blockElementGetter, lastBlockElementSupplier);
				handlers::RegisterPullBlocksHandler(pFsmShared->packetHandlers(), state.storage(), CreatePullBlocksHandlerConfiguration(config.Node));
				pFsmShared->dbrbProcess().registerDbrbPushNodesHandler(config.Immutable.NetworkIdentifier, pFsmShared->packetHandlers());
				pFsmShared->dbrbProcess().registerDbrbPullNodesHandler(pFsmShared->packetHandlers());
				pFsmShared->dbrbProcess().registerDbrbRemoveNodeRequestHandler(locator.keyPair(), pFsmShared->packetHandlers());
				pFsmShared->dbrbProcess().registerDbrbRemoveNodeResponseHandler(pFsmShared->packetHandlers());

				auto pReaders = pServiceGroup->pushService(net::CreatePacketReaders, pFsmShared->packetHandlers(), locator.keyPair(), connectionSettings, 2u);
				extensions::BootServer(*pServiceGroup, config.Node.DbrbPort, Service_Id, config, [&acceptor = *pReaders](
					const auto& socketInfo,
					const auto& callback) {
					acceptor.accept(socketInfo, callback);
				});
				locator.registerService(Readers_Service_Name, pReaders);

				auto& fastFinalityData = pFsmShared->fastFinalityData();
				fastFinalityData.setUnlockedAccounts(pUnlockedAccounts);
				fastFinalityData.setBeneficiary(crypto::ParseKey(m_harvestingConfig.Beneficiary));
				auto& actions = pFsmShared->actions();
				auto remoteNodeStateRetriever = CreateRemoteNodeStateRetriever<FastFinalityFsm>(pFsmShared, pConfigHolder, lastBlockElementSupplier);

				actions.CheckLocalChain = CreateFastFinalityCheckLocalChainAction(pFsmShared, state, remoteNodeStateRetriever, pConfigHolder, lastBlockElementSupplier, importanceGetter, m_dbrbConfig);
				actions.ResetLocalChain = CreateFastFinalityResetLocalChainAction();
				actions.DownloadBlocks = CreateFastFinalityDownloadBlocksAction(pFsmShared, state, blockRangeConsumer);
				actions.DetectRound = CreateFastFinalityDetectRoundAction(pFsmShared, lastBlockElementSupplier, state);
				actions.CheckConnections = CreateFastFinalityCheckConnectionsAction(pFsmShared, state);
				actions.SelectBlockProducer = CreateFastFinalitySelectBlockProducerAction(pFsmShared, state);
				actions.GenerateBlock = CreateFastFinalityGenerateBlockAction(pFsmShared, state.cache(), pConfigHolder, CreateHarvesterBlockGenerator(state), lastBlockElementSupplier);
				actions.WaitForBlock = CreateFastFinalityWaitForBlockAction(pFsmShared, pConfigHolder);
				actions.CommitBlock = CreateFastFinalityCommitBlockAction(pFsmShared, blockRangeConsumer, state);
				actions.IncrementRound = CreateFastFinalityIncrementRoundAction(pFsmShared, pConfigHolder);
				actions.ResetRound = CreateFastFinalityResetRoundAction(pFsmShared, state);

				m_pTransactionSender.reset();
				pFsmShared->start();
			}

		private:
			harvesting::HarvestingConfiguration m_harvestingConfig;
			dbrb::DbrbConfiguration m_dbrbConfig;
			std::shared_ptr<dbrb::TransactionSender> m_pTransactionSender;
		};
	}

	DECLARE_SERVICE_REGISTRAR(FastFinality)(
			const harvesting::HarvestingConfiguration& harvestingConfig,
			const dbrb::DbrbConfiguration& dbrbConfig,
			std::shared_ptr<dbrb::TransactionSender> pTransactionSender) {
		return std::make_unique<FastFinalityServiceRegistrar>(harvestingConfig, dbrbConfig, std::move(pTransactionSender));
	}
}}
