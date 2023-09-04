/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#include "RecoveryOrchestrator.h"
#include "CatapultSystemState.h"
#include "MultiBlockLoader.h"
#include "RecoveryStorageAdapter.h"
#include "RepairSpooling.h"
#include "RepairState.h"
#include "StateChangeRepairingSubscriber.h"
#include "StorageStart.h"
#include "catapult/config/CatapultDataDirectory.h"
#include "catapult/extensions/LocalNodeChainScore.h"
#include "catapult/extensions/LocalNodeStateFileStorage.h"
#include "catapult/extensions/LocalNodeStateRef.h"
#include "catapult/extensions/ProcessBootstrapper.h"
#include "catapult/io/BlockStorageCache.h"
#include "catapult/io/FilesystemUtils.h"
#include "catapult/io/MoveBlockFiles.h"
#include "catapult/local/HostUtils.h"
#include "catapult/subscribers/BlockChangeReader.h"
#include "catapult/subscribers/BrokerMessageReaders.h"
#include "catapult/subscribers/TransactionStatusReader.h"
#include "catapult/utils/StackLogger.h"
#include "catapult/extensions/NemesisBlockLoader.h"
#include "plugins/txes/config/src/cache/NetworkConfigCache.h"
#include "catapult/config/ConfigurationFileLoader.h"

namespace catapult { namespace local {

	namespace {
		// region DualStateChangeSubscriber

		class DualStateChangeSubscriber final : public subscribers::StateChangeSubscriber {
		public:
			DualStateChangeSubscriber(subscribers::StateChangeSubscriber& subscriber1, subscribers::StateChangeSubscriber& subscriber2)
					: m_subscriber1(subscriber1)
					, m_subscriber2(subscriber2)
			{}

		public:
			void notifyScoreChange(const model::ChainScore& chainScore) override {
				m_subscriber1.notifyScoreChange(chainScore);
				m_subscriber2.notifyScoreChange(chainScore);
			}

			void notifyStateChange(const subscribers::StateChangeInfo& stateChangeInfo) override {
				m_subscriber1.notifyStateChange(stateChangeInfo);
				m_subscriber2.notifyStateChange(stateChangeInfo);
			}

		private:
			subscribers::StateChangeSubscriber& m_subscriber1;
			subscribers::StateChangeSubscriber& m_subscriber2;
		};

		// endregion

		std::unique_ptr<io::PrunableBlockStorage> CreateStagingBlockStorage(const config::CatapultDataDirectory& dataDirectory) {
			auto stagingDirectory = dataDirectory.spoolDir("block_recover").str();
			boost::filesystem::create_directory(stagingDirectory);
			return std::make_unique<io::FileBlockStorage>(stagingDirectory, io::FileBlockStorageMode::None);
		}

		void MoveSupplementalDataFiles(const config::CatapultDataDirectory& dataDirectory) {
			if (!boost::filesystem::exists(dataDirectory.dir("state.tmp").path()))
				return;

			extensions::LocalNodeStateSerializer serializer(dataDirectory.dir("state.tmp"));
			serializer.moveTo(dataDirectory.dir("state"));
		}

		void MoveBlockFiles(const config::CatapultDirectory& stagingDirectory, io::BlockStorage& destinationStorage) {
			io::FileBlockStorage staging(stagingDirectory.str());
			if (Height(0) == staging.chainHeight())
				return;

			// mind that startHeight will be > 1, so there is no additional check
			auto startHeight = FindStartHeight(staging);
			CATAPULT_LOG(debug) << "moving blocks: " << startHeight << "-" << staging.chainHeight();
			io::MoveBlockFiles(staging, destinationStorage, startHeight);
		}

		class DefaultRecoveryOrchestrator final : public RecoveryOrchestrator {
		public:
			explicit DefaultRecoveryOrchestrator(std::unique_ptr<extensions::ProcessBootstrapper>&& pBootstrapper)
					: m_pBootstrapper(std::move(pBootstrapper))
					, m_dataDirectory(config::CatapultDataDirectoryPreparer::Prepare(m_pBootstrapper->config().User.DataDirectory))
					, m_catapultCache({}) // note that sub caches are added in boot
					, m_pBlockStorage(m_pBootstrapper->subscriptionManager().createBlockStorage(m_pBlockChangeSubscriber))
					, m_storage(CreateReadOnlyStorageAdapter(*m_pBlockStorage), CreateStagingBlockStorage(m_dataDirectory))
					, m_pTransactionStatusSubscriber(m_pBootstrapper->subscriptionManager().createTransactionStatusSubscriber())
					, m_pStateChangeSubscriber(m_pBootstrapper->subscriptionManager().createStateChangeSubscriber())
					, m_pluginManager(m_pBootstrapper->pluginManager())
			{}

			~DefaultRecoveryOrchestrator() override {
				shutdown();
			}

		public:
			void boot() {
				/// Initially we start by loading system requirements
				CATAPULT_LOG(info) << "registering system plugins";
				m_pluginModules = LoadSystemPlugins(*m_pBootstrapper);

				/// First we must determine if this is the first boot. If it is we must load nemesis block
				auto isFirstBoot = IsStatePresent(m_dataDirectory);
				std::unique_ptr<const model::NetworkConfiguration> initNetworkConfig;
				std::unique_ptr<const config::SupportedEntityVersions> initSupportedEntities;
				/// We must retrieve the configuration from the nemesis block
				auto storageView = m_storage.view();
				//This is the first boot, so we must load the nemesis block network configuration
				const auto pNemesisBlockElement = storageView.loadBlockElement(Height(1));
				auto bundleConfig = extensions::NemesisBlockLoader::ReadNetworkConfiguration(pNemesisBlockElement, m_pluginManager.immutableConfig());
				initSupportedEntities = std::make_unique<const config::SupportedEntityVersions>(std::get<1>(bundleConfig));
				if(isFirstBoot) {
					/// We set base config as the one in nemesis block.
					initNetworkConfig = std::make_unique<const model::NetworkConfiguration>(std::get<0>(bundleConfig));
				}
				else {
					auto stateDir = m_dataDirectory.dir("state");
					/// We must retrieve the configuration from the active config file if available
					/// At this point the configurations for the plugins are still uninitialized
					if(extensions::HasActiveNetworkConfig(stateDir)) {
						auto config = extensions::LoadActiveNetworkConfig(stateDir, m_pluginManager.immutableConfig());
						initNetworkConfig = std::make_unique<const model::NetworkConfiguration>(config);
					} else {
						/// Not first boot but no config was saved yet, so we will load the file from resources folder. This happens when first transitioning to this feature, but booting from an incomplete snapshot.
						initNetworkConfig = std::make_unique<model::NetworkConfiguration>(config::LoadIniConfiguration<model::NetworkConfiguration>(boost::filesystem::path(m_pBootstrapper->resourcesPath()) / config::Qualify("network"), m_pluginManager.immutableConfig()));
					}
				}

				// At this point we have managed to obtain the proper network configuration that we should boot up the chain with. We will load remaining plugins.
				/// Initialize proper zero network config.
				CATAPULT_LOG(debug) << "initializing network configuration from previous state";
				m_pBootstrapper->configHolder()->InitializeNetworkConfiguration(*initNetworkConfig, *initSupportedEntities);

				/// Load non system plugins
				CATAPULT_LOG(debug) << "registering addon plugins";
				auto addonPlugins = LoadConfigurablePlugins(*m_pBootstrapper);
				m_pluginModules.insert(m_pluginModules.cend(), addonPlugins.begin(), addonPlugins.end());

				/// New caches have been registered so we must update the cache.

				CATAPULT_LOG(debug) << "initializing system cache";
				CATAPULT_LOG(debug) << "initializing addon plugins cache";
				m_catapultCache = m_pluginManager.createCache();
				m_pluginManager.configHolder()->SetCache(&m_catapultCache);

				/// Finally we can build and run the plugin initializers to manipulate the config.

				auto initializers = m_pluginManager.createPluginInitializer();
				initializers(const_cast<model::NetworkConfiguration&>(m_pluginManager.configHolder()->Config().Network));
				m_pluginManager.configHolder()->SetPluginInitializer(initializers);

				utils::StackLogger stackLogger("running recovery operations", utils::LogLevel::Info);
				recover();
			}

		private:
			void recover() {
				auto systemState = CatapultSystemState(m_dataDirectory);

				CATAPULT_LOG(info) << "repairing spooling, commit step " << utils::to_underlying_type(systemState.commitStep());
				RepairSpooling(m_dataDirectory, systemState.commitStep());

				CATAPULT_LOG(info) << "repairing messages";
				repairSubscribers();

				CATAPULT_LOG(info) << "loading state";
				auto heights = extensions::LoadStateFromDirectory(m_dataDirectory.dir("state"), stateRef(), m_pluginManager);
				if (heights.Cache > heights.Storage)
					CATAPULT_THROW_RUNTIME_ERROR_2("cache height is larger than storage height", heights.Cache, heights.Storage);

				if (!stateRef().ConfigHolder->Config().Node.ShouldUseCacheDatabaseStorage)
					repairStateFromStorage(heights);

				CATAPULT_LOG(info) << "loaded block chain (height = " << heights.Storage << ", score = " << m_score.get() << ")";

				CATAPULT_LOG(info) << "repairing state";
				repairState(systemState.commitStep());

				CATAPULT_LOG(info) << "finalizing";
				systemState.reset();
			}

			void repairSubscribers() {
				// due to behavior of SubscriptionManager, block change subscriber is only subscriber that can be nullptr
				if (m_pBlockChangeSubscriber)
					processMessages("block_change", *m_pBlockChangeSubscriber, subscribers::ReadNextBlockChange);

				processMessages("transaction_status", *m_pTransactionStatusSubscriber, subscribers::ReadNextTransactionStatus);
			}

			template<typename TSubscriber, typename TMessageReader>
			void processMessages(const std::string& queueName, TSubscriber& subscriber, TMessageReader readNextMessage) {
				subscribers::ReadAll(
						{ m_dataDirectory.spoolDir(queueName).str(), "index_broker_r.dat", "index.dat" },
						subscriber,
						readNextMessage);
			}

			void repairStateFromStorage(const extensions::StateHeights& heights) {
				if (heights.Cache == heights.Storage)
					return;

				// disable load optimizations (loading from the saved state is optimization enough) in order to prevent
				// discontinuities in block analysis (e.g. difficulty cache expects consecutive blocks)
				CATAPULT_LOG(info) << "loading state - block loading required";
				auto observerFactory = [&pluginManager = m_pluginManager](const auto&) { return pluginManager.createObserver(); };
				auto partialScore = LoadBlockChain(observerFactory, m_pluginManager, stateRef(), heights.Cache + Height(1));
				m_score += partialScore;
			}

			void repairState(consumers::CommitOperationStep commitStep) {
				// RepairState always needs to be called in order to recover broker messages
				std::unique_ptr<subscribers::StateChangeSubscriber> pStateChangeRepairSubscriber;
				std::unique_ptr<subscribers::StateChangeSubscriber> pDualStateChangeSubscriber;
				const auto& config = stateRef().ConfigHolder->Config();
				if (config.Node.ShouldUseCacheDatabaseStorage) {
					pStateChangeRepairSubscriber = CreateStateChangeRepairingSubscriber(stateRef().Cache, stateRef().Score);
					pDualStateChangeSubscriber = std::make_unique<DualStateChangeSubscriber>(
							*pStateChangeRepairSubscriber,
							*m_pStateChangeSubscriber);
				}

				auto& repairSubscriber = config.Node.ShouldUseCacheDatabaseStorage
						? *pDualStateChangeSubscriber
						: *m_pStateChangeSubscriber;
				RepairState(m_dataDirectory.spoolDir("state_change"), stateRef().Cache, *m_pStateChangeSubscriber, repairSubscriber);

				if (consumers::CommitOperationStep::State_Written != commitStep) {
					CATAPULT_LOG(debug) << " - purging state.tmp";
					io::PurgeDirectory(m_dataDirectory.dir("state.tmp").str());
					return;
				}

				CATAPULT_LOG(debug) << " - moving supplemental data and block files";
				MoveSupplementalDataFiles(m_dataDirectory);
				MoveBlockFiles(m_dataDirectory.spoolDir("block_sync"), *m_pBlockStorage);
			}

		public:
			void shutdown() override {
				utils::StackLogger stackLogger("shutting down recovery orchestrator", utils::LogLevel::Info);

				m_pBootstrapper->pool().shutdown();
				saveStateToDisk();
			}

		private:
			void saveStateToDisk() {
				constexpr auto SaveStateToDirectoryWithCheckpointing = extensions::SaveStateToDirectoryWithCheckpointing;
				SaveStateToDirectoryWithCheckpointing(m_dataDirectory, m_pBootstrapper->config().Node, m_catapultCache, m_catapultState, m_score.get());
			}

		public:
			model::ChainScore score() const override {
				return m_score.get();
			}

		private:
			extensions::LocalNodeStateRef stateRef() {
				return extensions::LocalNodeStateRef(m_pBootstrapper->configHolder(), m_catapultState, m_catapultCache, m_storage, m_score);
			}

		private:
			// make sure modules are unloaded last
			std::vector<plugins::PluginModule> m_pluginModules;
			std::unique_ptr<extensions::ProcessBootstrapper> m_pBootstrapper;

			io::BlockChangeSubscriber* m_pBlockChangeSubscriber;
			config::CatapultDataDirectory m_dataDirectory;

			cache::CatapultCache m_catapultCache;
			state::CatapultState m_catapultState;
			std::unique_ptr<io::BlockStorage> m_pBlockStorage;
			io::BlockStorageCache m_storage;
			extensions::LocalNodeChainScore m_score;

			std::unique_ptr<subscribers::TransactionStatusSubscriber> m_pTransactionStatusSubscriber;
			std::unique_ptr<subscribers::StateChangeSubscriber> m_pStateChangeSubscriber;

			plugins::PluginManager& m_pluginManager;
		};
	}

	std::unique_ptr<RecoveryOrchestrator> CreateRecoveryOrchestrator(std::unique_ptr<extensions::ProcessBootstrapper>&& pBootstrapper) {
		return CreateAndBootHost<DefaultRecoveryOrchestrator>(std::move(pBootstrapper));
	}
}}
