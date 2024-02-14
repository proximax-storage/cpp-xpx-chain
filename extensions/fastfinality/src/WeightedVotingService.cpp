/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "WeightedVotingService.h"

#include <utility>
#include "WeightedVotingFsm.h"
#include "dbrb/DbrbPacketHandlers.h"
#include "catapult/api/ApiTypes.h"
#include "catapult/cache_core/ImportanceView.h"
#include "catapult/crypto/KeyUtils.h"
#include "catapult/extensions/ExecutionConfigurationFactory.h"
#include "catapult/extensions/NetworkUtils.h"
#include "catapult/extensions/ServiceLocator.h"
#include "catapult/handlers/ChainHandlers.h"
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
				CATAPULT_LOG(info) << "Added account " << publicKey << " for harvesting with result " << unlockResult;
			}

			return pUnlockedAccounts;
		}

		class RemoteRequestDispatcher {
		public:
			/// Creates a remote request dispatcher around \a io.
			explicit RemoteRequestDispatcher(ionet::NodePacketIoPair nodePacketIoPair, dbrb::MessageSender& messageSender)
				: m_nodePacketIoPair(std::move(nodePacketIoPair))
				, m_messageSender(messageSender)
			{}

		public:
			const ionet::NodePacketIoPair& nodePacketIoPair() {
				return m_nodePacketIoPair;
			}

		public:
			/// Dispatches \a args to the underlying io.
			template<typename TFuncTraits, typename... TArgs>
			thread::future<typename TFuncTraits::ResultType> dispatch(const TFuncTraits& traits, TArgs&&... args) {
				auto pPromise = std::make_shared<thread::promise<typename TFuncTraits::ResultType>>();
				auto future = pPromise->get_future();
				auto packetPayload = TFuncTraits::CreateRequestPacketPayload(std::forward<TArgs>(args)...);
				send(traits, packetPayload, [pPromise](auto result, auto&& value) {
					if (RemoteChainResult::Success == result) {
						pPromise->set_value(std::forward<decltype(value)>(value));
						return;
					}

					std::ostringstream message;
					message << GetErrorMessage(result) << " for " << TFuncTraits::Friendly_Name << " request";
					CATAPULT_LOG(error) << message.str();
					pPromise->set_exception(std::make_exception_ptr(api::catapult_api_error(message.str().data())));
				});

				return future;
			}

		private:
			template<typename TFuncTraits, typename TCallback>
			void send(const TFuncTraits& traits, const ionet::PacketPayload& packetPayload, const TCallback& callback) {
				using ResultType = typename TFuncTraits::ResultType;
				m_nodePacketIoPair.io()->write(packetPayload, [traits, callback, &io = *m_nodePacketIoPair.io(), &node = m_nodePacketIoPair.node(), &messageSender = m_messageSender](auto code) {
					if (ionet::SocketOperationCode::Success != code) {
						CATAPULT_LOG(trace) << "[REQUEST DISPATCHER] removing node " << node << " " << node.identityKey();
						messageSender.removeNode(node.identityKey());
						return callback(RemoteChainResult::Write_Error, ResultType());
					}

					io.read([traits, callback, &node, &messageSender](auto readCode, const auto* pResponsePacket) {
						if (ionet::SocketOperationCode::Success != readCode) {
							CATAPULT_LOG(trace) << "[REQUEST DISPATCHER] removing node " << node << " " << node.identityKey();
							messageSender.removeNode(node.identityKey());
							return callback(RemoteChainResult::Read_Error, ResultType());
						}

						if (TFuncTraits::Packet_Type != pResponsePacket->Type) {
							CATAPULT_LOG(warning) << "received packet of type " << pResponsePacket->Type << " but expected type " << TFuncTraits::Packet_Type;
							return callback(RemoteChainResult::Malformed_Packet, ResultType());
						}

						ResultType result;
						if (!traits.tryParseResult(*pResponsePacket, result)) {
							CATAPULT_LOG(warning) << "unable to parse " << pResponsePacket->Type << " packet (size = " << pResponsePacket->Size << ")";
							return callback(RemoteChainResult::Malformed_Packet, ResultType());
						}

						return callback(RemoteChainResult::Success, std::move(result));
					});
				});
			}

		private:
			enum class RemoteChainResult {
				Success,
				Write_Error,
				Read_Error,
				Malformed_Packet
			};

			static constexpr const char* GetErrorMessage(RemoteChainResult result) {
				switch (result) {
				case RemoteChainResult::Write_Error:
					return "write to remote node failed";
				case RemoteChainResult::Read_Error:
					return "read from remote node failed";
				default:
					return "remote node returned malformed packet";
				}
			}

		private:
			ionet::NodePacketIoPair m_nodePacketIoPair;
			dbrb::MessageSender& m_messageSender;
		};

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

					const auto& pDbrbProcess = pFsmShared->dbrbProcess();
					const auto& view = pDbrbProcess->currentView();
					auto minOpinionNumber = view.quorumSize() - 1;
					auto pMessageSender = pDbrbProcess->messageSender();
					pMessageSender->clearQueue();
					for (const auto& identityKey : view.Data) {
						if (identityKey == pDbrbProcess->id())
							continue;

						auto nodePacketIoPair = pMessageSender->getNodePacketIoPair(identityKey);
						if (nodePacketIoPair) {
							auto pDispatcher = std::make_shared<RemoteRequestDispatcher>(std::move(nodePacketIoPair), *pMessageSender);
							remoteNodeStateFutures.push_back(pDispatcher->dispatch(RemoteNodeStateTraits{}, targetHeight).then([pMessageSender, pDispatcher, identityKey](auto&& stateFuture) {
								auto remoteNodeState = stateFuture.get();
								remoteNodeState.NodeKey = identityKey;
								pMessageSender->pushNodePacketIoPair(identityKey, pDispatcher->nodePacketIoPair());
								return remoteNodeState;
							}));
						} else {
							CATAPULT_LOG(debug) << "got no packet io to request node state from " << identityKey;
						}
					}

					std::vector<RemoteNodeState> nodeStates;
					if (!remoteNodeStateFutures.empty()) {
						nodeStates = thread::when_all(std::move(remoteNodeStateFutures)).then([](auto&& completedFutures) {
							return thread::get_all_ignore_exceptional(completedFutures.get());
						}).get();
					}
					CATAPULT_LOG(debug) << "retrieved " << nodeStates.size() << " node states, min opinion number " << minOpinionNumber;

					if (nodeStates.size() < minOpinionNumber)
						nodeStates.clear();

					pPromise->set_value(std::move(nodeStates));
				});

				auto value = pPromise->get_future().get();

				return value;
			};
		}

		auto CreateHarvesterBlockGenerator(extensions::ServiceState& state) {
			auto strategy = state.config().Node.TransactionSelectionStrategy;
			auto executionConfig = extensions::CreateExecutionConfiguration(state.pluginManager());
			harvesting::HarvestingUtFacadeFactory utFacadeFactory(state.cache(), executionConfig);

			return harvesting::CreateHarvesterBlockGenerator(strategy, utFacadeFactory, state.utCache());
		}

		auto CreatePullBlocksHandlerConfiguration(const config::NodeConfiguration& nodeConfig) {
			handlers::PullBlocksHandlerConfiguration config {
				nodeConfig.MaxBlocksPerSyncAttempt,
				nodeConfig.MaxChainBytesPerSyncAttempt.bytes32(),
			};

			return config;
		}

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
					auto pDbrbProcess = std::make_shared<dbrb::DbrbProcess>(config::ToLocalNode(config), keyPair, state.nodes(), pWritersWeak, pDbrbPool, pTransactionSender, state.pluginManager().dbrbViewFetcher(), dbrbConfig);
					return std::make_shared<WeightedVotingFsm>(pWeightedVotingFsmPool, config, pDbrbProcess, state.pluginManager());
				});

				const auto& pluginManager = state.pluginManager();
				pFsmShared->dbrbProcess()->registerPacketHandlers(pFsmShared->packetHandlers());
				std::weak_ptr<WeightedVotingFsm> pFsmWeak = pFsmShared;
				pFsmShared->dbrbProcess()->setDeliverCallback([pFsmWeak, &pluginManager](const std::shared_ptr<ionet::Packet>& pPacket) {
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
				pFsmShared->dbrbProcess()->setValidationCallback([pFsmWeak, &pluginManager, &state, lastBlockElementSupplier, pValidatorPool](const std::shared_ptr<ionet::Packet>& pPacket) {
					auto pFsmShared = pFsmWeak.lock();
					if (!pFsmShared || pFsmShared->stopped())
						return false;

					switch (pPacket->Type) {
						case ionet::PacketType::Push_Proposed_Block: {
							return ValidateProposedBlock(*pFsmShared, *pPacket, state, lastBlockElementSupplier, pValidatorPool);
						}
						case ionet::PacketType::Push_Confirmed_Block: {
							return ValidateConfirmedBlock(*pFsmShared, *pPacket, state, lastBlockElementSupplier, pValidatorPool);
						}
						case ionet::PacketType::Push_Prevote_Messages: {
							return ValidatePrevoteMessages(*pFsmShared, *pPacket);
						}
						case ionet::PacketType::Push_Precommit_Messages: {
							return ValidatePrecommitMessages(*pFsmShared, *pPacket);
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

				RegisterPullRemoteNodeStateHandler(pFsmShared, pFsmShared->packetHandlers(), locator.keyPair().publicKey(), blockElementGetter, lastBlockElementSupplier);
				handlers::RegisterPullBlocksHandler(pFsmShared->packetHandlers(), state.storage(), CreatePullBlocksHandlerConfiguration(config.Node));
				dbrb::RegisterPushNodesHandler(pFsmShared->dbrbProcess(), config.Immutable.NetworkIdentifier, pFsmShared->packetHandlers());

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
					m_dbrbConfig);
				actions.ResetLocalChain = CreateDefaultResetLocalChainAction();
				actions.DownloadBlocks = CreateDefaultDownloadBlocksAction(pFsmShared, state, blockRangeConsumer);
				actions.DetectStage = CreateDefaultDetectStageAction(pFsmShared, state.timeSupplier(), lastBlockElementSupplier, state);
				actions.SelectCommittee = CreateDefaultSelectCommitteeAction(pFsmShared, state);
				actions.ProposeBlock = CreateDefaultProposeBlockAction(
					pFsmShared,
					state.cache(),
					pConfigHolder,
					CreateHarvesterBlockGenerator(state),
					lastBlockElementSupplier);
				actions.WaitForProposal = CreateDefaultWaitForProposalAction(pFsmShared);
				actions.WaitForPrevotes = CreateDefaultWaitForPrevotesAction(pFsmShared);
				actions.WaitForPrecommits = CreateDefaultWaitForPrecommitsAction(pFsmShared);
				actions.AddPrevote = CreateDefaultAddPrevoteAction(pFsmShared);
				actions.AddPrecommit = CreateDefaultAddPrecommitAction(pFsmShared);
				actions.UpdateConfirmedBlock = CreateDefaultUpdateConfirmedBlockAction(pFsmShared, state);
				actions.WaitForConfirmedBlock = CreateDefaultWaitForConfirmedBlockAction(pFsmShared, state);
				actions.CommitConfirmedBlock = CreateDefaultCommitConfirmedBlockAction(pFsmShared, blockRangeConsumer, state);
				actions.IncrementRound = CreateDefaultIncrementRoundAction(pFsmShared, pConfigHolder);
				actions.ResetRound = CreateDefaultResetRoundAction(pFsmShared, state);

				m_pTransactionSender.reset();
				pFsmShared->start();
			}

		private:
			harvesting::HarvestingConfiguration m_harvestingConfig;
			dbrb::DbrbConfiguration m_dbrbConfig;
			std::shared_ptr<dbrb::TransactionSender> m_pTransactionSender;

		};

		class WeightedVotingShutdownServiceRegistrar : public extensions::ServiceRegistrar {
		public:
			explicit WeightedVotingShutdownServiceRegistrar(std::shared_ptr<dbrb::TransactionSender> pTransactionSender)
				: m_pTransactionSender(std::move(pTransactionSender))
			{}

			class DbrbNodeRemover {
			public:
				explicit DbrbNodeRemover(std::shared_ptr<dbrb::TransactionSender> pTransactionSender, const plugins::PluginManager& pluginManager, const dbrb::ProcessId& id)
					: m_pTransactionSender(std::move(pTransactionSender))
					, m_pluginManager(pluginManager)
					, m_id(id)
				{}

			public:
				void shutdown() {
					if (m_pluginManager.config().EnableRemovingDbrbProcessOnShutdown) {
						m_pTransactionSender->disableAddDbrbProcessTransaction();
						auto view = m_pluginManager.dbrbViewFetcher().getView(utils::NetworkTime());
						if (m_pTransactionSender->isAddDbrbProcessTransactionSent() || view.find(m_id) != view.cend()) {
							auto future = m_pTransactionSender->sendRemoveDbrbProcessTransaction();
							future.wait_for(std::chrono::minutes(1));
						}
					}
				}

			private:
				std::shared_ptr<dbrb::TransactionSender> m_pTransactionSender;
				const plugins::PluginManager& m_pluginManager;
				dbrb::ProcessId m_id;
			};

			extensions::ServiceRegistrarInfo info() const override {
				return { "WeightedVoting", extensions::ServiceRegistrarPhase::Shutdown_Handlers };
			}

			void registerServiceCounters(extensions::ServiceLocator& locator) override {
			}

			void registerServices(extensions::ServiceLocator& locator, extensions::ServiceState& state) override {
				auto pServiceGroup = state.pool().pushServiceGroup("DBRB node remover");
				pServiceGroup->registerService(std::make_shared<DbrbNodeRemover>(m_pTransactionSender, state.pluginManager(), locator.keyPair().publicKey()));
			}

		private:
			std::shared_ptr<dbrb::TransactionSender> m_pTransactionSender;
		};
	}

	DECLARE_SERVICE_REGISTRAR(WeightedVoting)(
			const harvesting::HarvestingConfiguration& harvestingConfig,
			const dbrb::DbrbConfiguration& dbrbConfig,
			std::shared_ptr<dbrb::TransactionSender> pTransactionSender) {
		return std::make_unique<WeightedVotingServiceRegistrar>(harvestingConfig, dbrbConfig, std::move(pTransactionSender));
	}

	DECLARE_SERVICE_REGISTRAR(WeightedVotingShutdown)(std::shared_ptr<dbrb::TransactionSender> pTransactionSender) {
		return std::make_unique<WeightedVotingShutdownServiceRegistrar>(std::move(pTransactionSender));
	}
}}
