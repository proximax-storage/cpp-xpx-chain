/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "WeightedVotingService.h"
#include "WeightedVotingFsm.h"
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
		constexpr auto Writers_Service_Name = "weightedvoting.writers";
		constexpr auto Readers_Service_Name = "weightedvoting.readers";
		constexpr auto Service_Id = ionet::ServiceIdentifier(0x54654144);

		class WeightedVotingServiceRegistrar : public extensions::ServiceRegistrar {
		public:
			explicit WeightedVotingServiceRegistrar(
					harvesting::HarvestingConfiguration  harvestingConfig,
					const dbrb::DbrbConfiguration& dbrbConfig,
					std::shared_ptr<dbrb::TransactionSender> pTransactionSender)
				: m_harvestingConfig(std::move(harvestingConfig))
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
				const auto& nextConfig = state.config(state.storage().view().chainHeight() + Height(1)).Network;
				if (!nextConfig.EnableWeightedVoting) {
					CATAPULT_LOG(warning) << "weighted voting is not enabled";
					return;
				}

				auto pValidatorPool = state.pool().pushIsolatedPool("proposal validator");
				auto pDbrbPool = state.pool().pushIsolatedPool("dbrb");
				auto pWeightedVotingFsmPool = state.pool().pushIsolatedPool("weighted voting fsm");
				auto pServiceGroup = state.pool().pushServiceGroup("weighted voting");

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
						pWeightedVotingFsmPool,
						dbrbShardingEnabled,
						dbrbShardSize](const std::shared_ptr<thread::IoThreadPool>&) {
					pTransactionSender->init(&keyPair, config.Immutable, dbrbConfig, state.hooks().transactionRangeConsumerFactory()(disruptor::InputSource::Local), pUnlockedAccounts);
					auto pMessageSender = dbrb::CreateMessageSender(config::ToLocalNode(config), state.nodes(), dbrbConfig.IsDbrbProcess, pDbrbPool, dbrbConfig.ResendMessagesInterval);
					if (dbrbShardingEnabled) {
						auto pDbrbProcess = std::make_shared<dbrb::ShardedDbrbProcess>(keyPair, pMessageSender, pDbrbPool, pTransactionSender, state.pluginManager().dbrbViewFetcher(), dbrbShardSize);
						return std::make_shared<WeightedVotingFsm>(pWeightedVotingFsmPool, config, pDbrbProcess, state.pluginManager());
					} else {
						auto pDbrbProcess = std::make_shared<dbrb::DbrbProcess>(keyPair, pMessageSender, pDbrbPool, pTransactionSender, state.pluginManager().dbrbViewFetcher());
						return std::make_shared<WeightedVotingFsm>(pWeightedVotingFsmPool, config, pDbrbProcess, state.pluginManager());
					}
				});

				const auto& pluginManager = state.pluginManager();
				pFsmShared->dbrbProcess().registerPacketHandlers(pFsmShared->packetHandlers());
				std::weak_ptr<WeightedVotingFsm> pFsmWeak = pFsmShared;
				pFsmShared->dbrbProcess().setDeliverCallback([pFsmWeak, &pluginManager](const std::shared_ptr<ionet::Packet>& pPacket) {
					TRY_GET_FSM()

					switch (pPacket->Type) {
						case ionet::PacketType::Push_Proposed_Block: {
							PushProposedBlock(*pFsmShared, pluginManager, *pPacket);
							break;
						}
						case ionet::PacketType::Push_Confirmed_Block: {
							PushConfirmedBlock(*pFsmShared, pluginManager, *pPacket);
							break;
						}
						case ionet::PacketType::Push_Prevote_Messages: {
							PushPrevoteMessages(*pFsmShared, *pPacket);
							break;
						}
						case ionet::PacketType::Push_Precommit_Messages: {
							PushPrecommitMessages(*pFsmShared, *pPacket);
							break;
						}
					}
				});

				const auto& storage = state.storage();
				auto lastBlockElementSupplier = [&storage]() {
					auto storageView = storage.view();
					return storageView.loadBlockElement(storageView.chainHeight());
				};
				pFsmShared->dbrbProcess().setValidationCallback([pFsmWeak, &pluginManager, &state, lastBlockElementSupplier, pValidatorPool](const std::shared_ptr<ionet::Packet>& pPacket, const Hash256&) {
					auto pFsmShared = pFsmWeak.lock();
					if (!pFsmShared || pFsmShared->stopped())
						return dbrb::MessageValidationResult::Message_Broadcast_Stopped;

					bool valid = false;
					switch (pPacket->Type) {
						case ionet::PacketType::Push_Proposed_Block: {
							valid = ValidateProposedBlock(*pFsmShared, *pPacket, state, lastBlockElementSupplier, pValidatorPool);
							break;
						}
						case ionet::PacketType::Push_Confirmed_Block: {
							valid = ValidateConfirmedBlock(*pFsmShared, *pPacket, state, lastBlockElementSupplier, pValidatorPool);
							break;
						}
						case ionet::PacketType::Push_Prevote_Messages: {
							valid = ValidatePrevoteMessages(*pFsmShared, *pPacket);
							break;
						}
						case ionet::PacketType::Push_Precommit_Messages: {
							valid = ValidatePrecommitMessages(*pFsmShared, *pPacket);
							break;
						}
					}

					return (valid ? dbrb::MessageValidationResult::Message_Valid : dbrb::MessageValidationResult::Message_Invalid);
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

				auto pReaders = pServiceGroup->pushService(net::CreatePacketReaders, pFsmShared->packetHandlers(), locator.keyPair(), connectionSettings, 2u);
				extensions::BootServer(*pServiceGroup, config.Node.DbrbPort, Service_Id, config, 1, [&acceptor = *pReaders](
					const auto& socketInfo,
					const auto& callback) {
					acceptor.accept(socketInfo, callback);
				});
				locator.registerService(Readers_Service_Name, pReaders);

				auto& committeeData = pFsmShared->committeeData();
				committeeData.setUnlockedAccounts(pUnlockedAccounts);
				committeeData.setBeneficiary(crypto::ParseKey(m_harvestingConfig.Beneficiary));
				auto& actions = pFsmShared->actions();

				actions.CheckLocalChain = CreateWeightedVotingCheckLocalChainAction(
					pFsmShared,
					CreateRemoteNodeStateRetriever<WeightedVotingFsm>(pFsmShared, pConfigHolder, lastBlockElementSupplier),
					pConfigHolder,
					lastBlockElementSupplier,
					importanceGetter,
					m_dbrbConfig);
				actions.ResetLocalChain = CreateWeightedVotingResetLocalChainAction();
				actions.DownloadBlocks = CreateWeightedVotingDownloadBlocksAction(pFsmShared, state, blockRangeConsumer);
				actions.DetectStage = CreateWeightedVotingDetectStageAction(pFsmShared, state.timeSupplier(), lastBlockElementSupplier, state);
				actions.SelectCommittee = CreateWeightedVotingSelectCommitteeAction(pFsmShared, state);
				actions.ProposeBlock = CreateWeightedVotingProposeBlockAction(
					pFsmShared,
					state,
					pConfigHolder,
					CreateHarvesterBlockGenerator(state),
					lastBlockElementSupplier);
				actions.WaitForProposal = CreateWeightedVotingWaitForProposalAction(pFsmShared);
				actions.WaitForPrevotes = CreateWeightedVotingWaitForPrevotesAction(pFsmShared);
				actions.WaitForPrecommits = CreateWeightedVotingWaitForPrecommitsAction(pFsmShared);
				actions.AddPrevote = CreateWeightedVotingAddPrevoteAction(pFsmShared, state);
				actions.AddPrecommit = CreateWeightedVotingAddPrecommitAction(pFsmShared, state);
				actions.UpdateConfirmedBlock = CreateWeightedVotingUpdateConfirmedBlockAction(pFsmShared, state);
				actions.WaitForConfirmedBlock = CreateWeightedVotingWaitForConfirmedBlockAction(pFsmShared, state);
				actions.CommitConfirmedBlock = CreateWeightedVotingCommitConfirmedBlockAction(pFsmShared, blockRangeConsumer, state);
				actions.IncrementRound = CreateWeightedVotingIncrementRoundAction(pFsmShared, pConfigHolder);
				actions.ResetRound = CreateWeightedVotingResetRoundAction(pFsmShared, state);

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
