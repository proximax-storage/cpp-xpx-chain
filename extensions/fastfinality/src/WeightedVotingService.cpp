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
		constexpr auto Dbrb_Writers_Service_Name = "dbrb.writers";
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
				const std::shared_ptr<dbrb::DbrbProcess>& pDbrbProcess) {
			return [pConfigHolder, lastBlockElementSupplier, &pDbrbProcess]() {
				std::vector<thread::future<RemoteNodeState>> remoteNodeStateFutures;
				auto timeout = utils::TimeSpan::FromSeconds(5);

				auto packetIoPairs = pDbrbProcess->getNodePacketIoPairs();
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

			void registerServiceCounters(extensions::ServiceLocator& locator) override {
				locator.registerServiceCounter<net::PacketWriters>(Writers_Service_Name, "WV WRITERS", [](const auto& writers) {
					return writers.numActiveWriters();
				});
				locator.registerServiceCounter<net::PacketWriters>(Dbrb_Writers_Service_Name, "DBRB WRITERS", [](const auto& writers) {
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
				auto pDbrbWriters = pServiceGroup->pushService(net::CreatePacketWriters, locator.keyPair(), connectionSettings, state);
				locator.registerService(Writers_Service_Name, pDbrbWriters);
				auto pWriters = pServiceGroup->pushService(net::CreatePacketWriters, locator.keyPair(), connectionSettings, state);
				locator.registerService(Dbrb_Writers_Service_Name, pWriters);

				auto nodeContainerView = state.nodes().view();
				std::vector<ionet::Node> bootstrapNodes;
				ionet::Node thisNode;
				bootstrapNodes.reserve(nodeContainerView.size());
				auto count = 0u;
				nodeContainerView.forEach([&bootstrapNodes, &thisNode, &count](const ionet::Node& node, const ionet::NodeInfo& info) {
					auto endPoint = node.endpoint();
					// TODO: get rid of hardcoded port value.
					endPoint.Port += Port_Diff;
					if (info.source() == ionet::NodeSource::Local) {
						thisNode = ionet::Node(node.identityKey(), endPoint, node.metadata());
					} else {
						bootstrapNodes.emplace_back(node.identityKey(), endPoint, node.metadata());
					}
				});

				auto pPacketHandlers = std::make_shared<ionet::ServerPacketHandlers>(config.Node.MaxPacketDataSize.bytes32());
				auto pDbrbProcess = std::make_shared<dbrb::DbrbProcess>(pDbrbWriters, pWriters, pPacketHandlers, bootstrapNodes, thisNode);

				auto pFsmShared = pServiceGroup->pushService([&state, &config, pDbrbProcess](const std::shared_ptr<thread::IoThreadPool>& pPool) {
					return std::make_shared<WeightedVotingFsm>(pPool, config, pDbrbProcess);
				});

				const auto& pluginManager = state.pluginManager();
				pDbrbProcess->setDeliverCallback([pFsmShared, &pluginManager](const std::shared_ptr<ionet::Packet>& pPacket) {
					switch (pPacket->Type) {
						case ionet::PacketType::Push_Proposed_Block: {
							PushProposedBlock(pFsmShared, pluginManager);
							break;
						}
						case ionet::PacketType::Push_Confirmed_Block: {
							PushConfirmedBlock(pFsmShared, pluginManager);
							break;
						}
						case ionet::PacketType::Push_Prevote_Messages: {
							PushPrevoteMessages(pFsmShared);
							break;
						}
						case ionet::PacketType::Push_Precommit_Messages: {
							PushPrecommitMessages(pFsmShared);
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

				RegisterPullRemoteNodeStateHandler(pFsmShared, *pPacketHandlers, pConfigHolder, blockElementGetter, lastBlockElementSupplier);

				auto pReaders = pServiceGroup->pushService(net::CreatePacketReaders, *pPacketHandlers, locator.keyPair(), connectionSettings, 2u);
				extensions::BootServer(*pServiceGroup, config.Node.Port + Port_Diff, Service_Id, config, pDbrbProcess->getNodeSubscriber(), [&acceptor = *pReaders](
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
					CreateRemoteNodeStateRetriever(pConfigHolder, lastBlockElementSupplier, pDbrbProcess),
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
				actions.ValidateProposal = CreateDefaultValidateProposalAction(
					pFsmShared,
					state,
					lastBlockElementSupplier,
					pValidatorPool);
				actions.WaitForProposalPhaseEnd = CreateDefaultWaitForProposalPhaseEndAction(pFsmShared);
				actions.WaitForPrevotePhaseEnd = CreateDefaultWaitForPrevotePhaseEndAction(pFsmShared, pluginManager.getCommitteeManager(), pConfigHolder);
				actions.AddPrevote = CreateDefaultAddPrevoteAction(pFsmShared);
				actions.AddPrecommit = CreateDefaultAddPrecommitAction(pFsmShared);
				actions.WaitForPrecommitPhaseEnd = CreateDefaultWaitForPrecommitPhaseEndAction(pFsmShared, pluginManager.getCommitteeManager(), pConfigHolder);
				actions.UpdateConfirmedBlock = CreateDefaultUpdateConfirmedBlockAction(pFsmShared, pluginManager.getCommitteeManager());
				actions.CommitConfirmedBlock = CreateDefaultCommitConfirmedBlockAction(
					pFsmShared,
					blockRangeConsumer,
					lastBlockElementSupplier,
					pConfigHolder,
					pluginManager.getCommitteeManager());
				actions.IncrementRound = CreateDefaultIncrementRoundAction(pFsmShared, pConfigHolder);
				actions.ResetRound = CreateDefaultResetRoundAction(pFsmShared, pConfigHolder, pluginManager.getCommitteeManager());

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
