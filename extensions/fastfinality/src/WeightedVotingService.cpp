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

		auto CreateRemoteChainHeightsRetriever(const net::PacketIoPickerContainer& packetIoPickers) {
			return [&packetIoPickers]() {
				std::vector<thread::future<Height>> heightFutures;
				auto timeout = utils::TimeSpan::FromSeconds(5);

				auto packetIoPairs = packetIoPickers.pickMultiple(timeout);
				if (packetIoPairs.empty()) {
					CATAPULT_LOG(warning) << "could not find any peer for detecting chain heights";
					return thread::make_ready_future(std::vector<Height>());
				}

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
				if (packetIoPairs.empty()) {
					CATAPULT_LOG(warning) << "could not find any peer for pulling block hashes";
					return thread::make_ready_future(std::vector<std::pair<Hash256, Key>>());
				}

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

		auto CreateRemoteCommitteeStagesRetriever(const net::PacketIoPickerContainer& packetIoPickers) {
			return [&packetIoPickers]() {
				std::vector<thread::future<CommitteeStage>> committeeStageFutures;
				auto timeout = utils::TimeSpan::FromSeconds(5);

				auto packetIoPairs = packetIoPickers.pickMultiple(timeout);
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

		auto CreateHarvesterBlockGenerator(extensions::ServiceState& state) {
			auto strategy = state.config().Node.TransactionSelectionStrategy;
			auto executionConfig = extensions::CreateExecutionConfiguration(state.pluginManager());
			harvesting::HarvestingUtFacadeFactory utFacadeFactory(state.cache(), executionConfig);

			return harvesting::CreateHarvesterBlockGenerator(strategy, utFacadeFactory, state.utCache());
		}

		class WeightedVotingService {
		public:
			explicit WeightedVotingService(std::shared_ptr<thread::IoThreadPool> pPool)
				: m_pFsm(std::make_shared<WeightedVotingFsm>(pPool))
				, m_strand(pPool->ioContext())
			{}

		public:
			void start() {
				boost::asio::post(m_strand, [this] {
					m_pFsm->start();
				});
			}

			void shutdown() {
				m_pFsm->shutdown();
				m_pFsm = nullptr;
			}

			const std::shared_ptr<WeightedVotingFsm>& fsm() {
				return m_pFsm;
			}

		private:
			std::shared_ptr<WeightedVotingFsm> m_pFsm;
			boost::asio::io_context::strand m_strand;
		};

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
				auto pService = pServiceGroup->pushService([](std::shared_ptr<thread::IoThreadPool> pPool) {
					return std::make_shared<WeightedVotingService>(pPool);
				});
				auto pFsmShared = pService->fsm();

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
				pluginManager.getCommitteeManager().setLastBlockElementSupplier(lastBlockElementSupplier);

				RegisterPullCommitteeStageHandler(pFsmShared, packetHandlers);
				RegisterPushProposedBlockHandler(pFsmShared, packetHandlers, pluginManager);
				RegisterPushConfirmedBlockHandler(pFsmShared, packetHandlers, pluginManager);
				RegisterPushPrevoteMessageHandler(pFsmShared, packetHandlers);
				RegisterPushPrecommitMessageHandler(pFsmShared, packetHandlers);

				auto& committeeData = pFsmShared->committeeData();
				committeeData.setUnlockedAccounts(CreateUnlockedAccounts(m_harvestingConfig));
				committeeData.setBeneficiary(crypto::ParseKey(m_harvestingConfig.Beneficiary));
				auto& actions = pFsmShared->actions();
				auto blockRangeConsumer = state.hooks().completionAwareBlockRangeConsumerFactory()(disruptor::InputSource::Remote_Pull);

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
					CreateRemoteCommitteeStagesRetriever(packetIoPickers),
					pConfigHolder,
					timeSupplier,
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
				actions.WaitForProposal = CreateDefaultWaitForProposalAction(pFsmShared, pConfigHolder, lastBlockElementSupplier);
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

				pService->start();
			}

		private:
			harvesting::HarvestingConfiguration m_harvestingConfig;
		};
	}

	DECLARE_SERVICE_REGISTRAR(WeightedVoting)(const harvesting::HarvestingConfiguration& config) {
		return std::make_unique<WeightedVotingServiceRegistrar>(config);
	}
}}
