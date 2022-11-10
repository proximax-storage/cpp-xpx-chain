/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "WeightedVotingService.h"
#include "WeightedVotingFsm.h"
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
		constexpr unsigned int Port_Diff = 3u;

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

		constexpr utils::LogLevel MapToLogLevel(net::PeerConnectCode connectCode) {
			if (connectCode == net::PeerConnectCode::Accepted)
				return utils::LogLevel::Info;
			else
				return utils::LogLevel::Warning;
		}

		thread::Task CreateConnectPeersTask(ionet::NodeContainer& nodes, net::PacketWriters& packetWriters, const Key& identityKey) {
			return thread::CreateNamedTask("connect peers task for service WeightedVoting", [&nodes, &packetWriters, identityKey]() {
				using ConnectResult = std::pair<ionet::Node, net::PeerConnectCode>;

				auto connectedNodes = packetWriters.identities();
				ionet::NodeSet addCandidates;
				nodes.view().forEach([&connectedNodes, &addCandidates, identityKey](const auto& node, const auto& nodeInfo) {
					const auto& endpoint = node.endpoint();
					if (endpoint.Port != 0 && identityKey != node.identityKey() && connectedNodes.find(node.identityKey()) == connectedNodes.end()) {
						unsigned short port = endpoint.Port + Port_Diff;
						addCandidates.insert(ionet::Node(node.identityKey(), ionet::NodeEndpoint{ endpoint.Host, port }, node.metadata()));
					}
				});

				if (addCandidates.size() == 0)
					return thread::make_ready_future(thread::TaskResult::Continue);

				auto i = 0u;
				std::vector<thread::future<ConnectResult>> futures(addCandidates.size());
				for (const auto& node : addCandidates) {
					auto pPromise = std::make_shared<thread::promise<ConnectResult>>();
					futures[i++] = pPromise->get_future();

					packetWriters.connect(node, [node, pPromise](const auto& connectResult) {
						pPromise->set_value(std::make_pair(node, connectResult.Code));
					});
				}

				thread::when_all(std::move(futures)).then([](auto&& connectResultsFuture) {
					for (auto& resultFuture : connectResultsFuture.get()) {
						auto connectResult = resultFuture.get();
						const auto& endpoint = connectResult.first.endpoint();
						CATAPULT_LOG_LEVEL(MapToLogLevel(connectResult.second))
							<< "connection attempt to " << connectResult.first << " @ " << endpoint.Host << " : " << endpoint.Port << " completed with " << connectResult.second;
					}
				});

				return thread::make_ready_future(thread::TaskResult::Continue);
			});
		}

		class WeightedVotingServiceRegistrar : public extensions::ServiceRegistrar {
		public:
			explicit WeightedVotingServiceRegistrar(const harvesting::HarvestingConfiguration& config) : m_harvestingConfig(config)
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
					CATAPULT_THROW_RUNTIME_ERROR("weighted voting is not enabled");

				auto pValidatorPool = state.pool().pushIsolatedPool("proposal validator");
				auto pServiceGroup = state.pool().pushServiceGroup("weighted voting");

				auto connectionSettings = extensions::GetConnectionSettings(config);
				auto pWriters = pServiceGroup->pushService(net::CreatePacketWriters, locator.keyPair(), connectionSettings, state);
				locator.registerService(Writers_Service_Name, pWriters);
				auto identityKey = crypto::KeyPair::FromString(config.User.BootKey).publicKey();
				state.tasks().push_back(CreateConnectPeersTask(state.nodes(), *pWriters, identityKey));

				auto pFsmShared = pServiceGroup->pushService([&state, &config](std::shared_ptr<thread::IoThreadPool> pPool) {
					return std::make_shared<WeightedVotingFsm>(pPool, config);
				});

				auto& packetIoPickers = pFsmShared->packetIoPickers();
				packetIoPickers.insert(*pWriters, ionet::NodeRoles::Peer);
				const auto& pluginManager = state.pluginManager();
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

				auto& packetHandlers = pFsmShared->packetHandlers();
				RegisterPushProposedBlockHandler(pFsmShared, packetHandlers, pluginManager);
				RegisterPushConfirmedBlockHandler(pFsmShared, packetHandlers, pluginManager);
				RegisterPushPrevoteMessagesHandler(pFsmShared, packetHandlers);
				RegisterPushPrecommitMessagesHandler(pFsmShared, packetHandlers);
				RegisterPullProposedBlockHandler(pFsmShared, packetHandlers);
				RegisterPullConfirmedBlockHandler(pFsmShared, packetHandlers);
				RegisterPullPrevoteMessagesHandler(pFsmShared, packetHandlers);
				RegisterPullPrecommitMessagesHandler(pFsmShared, packetHandlers);
				// TODO: Consider rewriting to (pFsmShared, packetHandlers, pluginManager, storage)
				RegisterPullRemoteNodeStateHandler(pFsmShared, packetHandlers, pConfigHolder, blockElementGetter, lastBlockElementSupplier);

				auto pReaders = pServiceGroup->pushService(net::CreatePacketReaders, packetHandlers, locator.keyPair(), connectionSettings, 2u);
				extensions::BootServer(*pServiceGroup, config.Node.Port + Port_Diff, Service_Id, config, state.nodeSubscriber(), [&acceptor = *pReaders](
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
				actions.RequestProposal = CreateDefaultRequestProposalAction(pFsmShared, state);
				actions.ValidateProposal = CreateDefaultValidateProposalAction(
					pFsmShared,
					state,
					lastBlockElementSupplier,
					pValidatorPool);
				actions.WaitForProposalPhaseEnd = CreateDefaultWaitForProposalPhaseEndAction(pFsmShared, pConfigHolder);
				actions.RequestPrevotes = CreateDefaultRequestPrevotesAction(pFsmShared, pluginManager);
				actions.RequestPrecommits = CreateDefaultRequestPrecommitsAction(pFsmShared, pluginManager);
				actions.AddPrevote = CreateDefaultAddPrevoteAction(pFsmShared);
				actions.AddPrecommit = CreateDefaultAddPrecommitAction(pFsmShared);
				actions.WaitForPrecommitPhaseEnd = CreateDefaultWaitForPrecommitPhaseEndAction(pFsmShared, pConfigHolder);
				actions.UpdateConfirmedBlock = CreateDefaultUpdateConfirmedBlockAction(pFsmShared, pluginManager.getCommitteeManager());
				actions.CommitConfirmedBlock = CreateDefaultCommitConfirmedBlockAction(
					pFsmShared,
					blockRangeConsumer,
					pConfigHolder,
					pluginManager.getCommitteeManager());
				actions.IncrementRound = CreateDefaultIncrementRoundAction(pFsmShared, pConfigHolder);
				actions.ResetRound = CreateDefaultResetRoundAction(pFsmShared, pConfigHolder, pluginManager.getCommitteeManager());
				actions.RequestConfirmedBlock = CreateDefaultRequestConfirmedBlockAction(pFsmShared, state, lastBlockElementSupplier);

				pFsmShared->start();
			}

		private:
			harvesting::HarvestingConfiguration m_harvestingConfig;
		};
	}

	DECLARE_SERVICE_REGISTRAR(WeightedVoting)(const harvesting::HarvestingConfiguration& config) {
		return std::make_unique<WeightedVotingServiceRegistrar>(config);
	}
}}
