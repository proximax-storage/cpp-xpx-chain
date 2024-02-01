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

#include "LocalNode.h"
#include "FileStateChangeStorage.h"
#include "MemoryCounters.h"
#include "NemesisBlockNotifier.h"
#include "NodeUtils.h"
#include "catapult/config/CatapultDataDirectory.h"
#include "catapult/extensions/ConfigurationUtils.h"
#include "catapult/extensions/LocalNodeChainScore.h"
#include "catapult/extensions/LocalNodeStateFileStorage.h"
#include "catapult/extensions/LocalNodeStateRef.h"
#include "catapult/extensions/ProcessBootstrapper.h"
#include "catapult/extensions/ServiceLocator.h"
#include "catapult/extensions/ServiceState.h"
#include "catapult/io/BlockStorageCache.h"
#include "catapult/io/FileQueue.h"
#include "catapult/ionet/NodeContainer.h"
#include "catapult/local/HostUtils.h"
#include "catapult/utils/StackLogger.h"
#include "catapult/extensions/NemesisBlockLoader.h"
#include "plugins/txes/config/src/cache/NetworkConfigCache.h"
#include "catapult/config/ConfigurationFileLoader.h"

namespace catapult { namespace local {

	namespace {
		std::unique_ptr<subscribers::StateChangeSubscriber> CreateStateChangeSubscriber(
				subscribers::SubscriptionManager& subscriptionManager,
				const extensions::CacheHolder& cacheHolder,
				const config::CatapultDataDirectory& dataDirectory) {
			subscriptionManager.addStateChangeSubscriber(CreateFileStateChangeStorage(
					std::make_unique<io::FileQueueWriter>(dataDirectory.spoolDir("state_change").str(), "index_server.dat"),
					[&cacheHolder]() { return cacheHolder.cache().changesStorages(); }));
			return subscriptionManager.createStateChangeSubscriber();
		}

		std::unique_ptr<subscribers::NodeSubscriber> CreateNodeSubscriber(
				subscribers::SubscriptionManager& subscriptionManager,
				ionet::NodeContainer& nodes) {
			subscriptionManager.addNodeSubscriber(CreateNodeContainerSubscriberAdapter(nodes));
			return subscriptionManager.createNodeSubscriber();
		}

		class DefaultLocalNode final : public LocalNode {
		public:
			DefaultLocalNode(std::unique_ptr<extensions::ProcessBootstrapper>&& pBootstrapper, const crypto::KeyPair& keyPair)
					: m_pBootstrapper(std::move(pBootstrapper))
					, m_serviceLocator(keyPair)
					, m_dataDirectory(config::CatapultDataDirectoryPreparer::Prepare(m_pBootstrapper->config().User.DataDirectory))
					, m_nodes(m_pBootstrapper->config().Node.MaxTrackedNodes, m_pBootstrapper->extensionManager().networkTimeSupplier())
					, m_cacheHolder(m_pBootstrapper->cacheHolder()) // note that sub caches are added in boot
					, m_storage(
							m_pBootstrapper->subscriptionManager().createBlockStorage(m_pBlockChangeSubscriber),
							CreateStagingBlockStorage(m_dataDirectory))
					, m_pUtCache(m_pBootstrapper->subscriptionManager().createUtCache(extensions::GetUtCacheOptions(m_pBootstrapper->config().Node)))
					, m_pTransactionStatusSubscriber(m_pBootstrapper->subscriptionManager().createTransactionStatusSubscriber())
					, m_pStateChangeSubscriber(CreateStateChangeSubscriber(
							m_pBootstrapper->subscriptionManager(),
							m_cacheHolder,
							m_dataDirectory))
					, m_pNodeSubscriber(CreateNodeSubscriber(m_pBootstrapper->subscriptionManager(), m_nodes))
					, m_pluginManager(m_pBootstrapper->pluginManager())
					, m_isBooted(false)
			{}

			~DefaultLocalNode() override {
				CATAPULT_CLEANUP_LOG(info, "Local Node is beginning shutdown procedure.");
				shutdown();
				CATAPULT_CLEANUP_LOG(info, "Local Node has shutdown, cleaning members.");
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
					CATAPULT_LOG(debug) << "no state found... assuming first boot";
					initNetworkConfig = std::make_unique<const model::NetworkConfiguration>(std::get<0>(bundleConfig));
				}
				else {
					auto stateDir = m_dataDirectory.dir("state");
					/// We must retrieve the configuration from the active config file if available
					/// At this point the configurations for the plugins are still uninitialized
					if(extensions::HasActiveNetworkConfig(stateDir)) {
						CATAPULT_LOG(debug) << "network configuration available, loading from stored state...";
						auto config = extensions::LoadActiveNetworkConfig(stateDir, m_pluginManager.immutableConfig());
						initNetworkConfig = std::make_unique<const model::NetworkConfiguration>(config);
					} else {
						CATAPULT_LOG(debug) << "network configuration unavailable, loading from resources...";
						/// Not first boot but no config was saved yet, so we will load the file from resources folder. This happens when first transitioning to this feature, but booting from an incomplete snapshot.
						initNetworkConfig = std::make_unique<model::NetworkConfiguration>(config::LoadIniConfiguration<model::NetworkConfiguration>(boost::filesystem::path(m_pBootstrapper->resourcesPath()) / config::Qualify("network"), m_pluginManager.immutableConfig()));
					}
				}

				/// At this point we have managed to obtain the proper network configuration that we should boot up the chain with. We will load remaining plugins.
				/// Initialize proper zero network config.
				CATAPULT_LOG(debug) << "initializing network configuration from previous state";
				m_pBootstrapper->configHolder()->InitializeNetworkConfiguration(*initNetworkConfig, *initSupportedEntities);

				/// Load non system plugins
				CATAPULT_LOG(debug) << "registering addon plugins";
				auto addonPlugins = LoadConfigurablePlugins(*m_pBootstrapper);
				m_pluginModules.insert(m_pluginModules.cend(), addonPlugins.begin(), addonPlugins.end());

				/// We can now create the cache with all avaiolable plugins.

				CATAPULT_LOG(debug) << "initializing addon plugins cache";
				CATAPULT_LOG(debug) << "initializing system cache";
				m_cacheHolder.cache() = m_pluginManager.createCache();
				m_pluginManager.configHolder()->SetCache(&m_cacheHolder.cache());

				/// Finally we can build and run the plugin initializers to manipulate the config.

				auto initializers = m_pluginManager.createPluginInitializer();
				initializers(const_cast<model::NetworkConfiguration&>(m_pluginManager.configHolder()->Config().Network));
				m_pluginManager.configHolder()->SetPluginInitializer(initializers);

				CATAPULT_LOG(debug) << "registering counters";
				registerCounters();

				utils::StackLogger stackLogger("booting local node", utils::LogLevel::Info);
				if(isFirstBoot) executeAndNotifyNemesis();
				loadStateFromDisk();

				CATAPULT_LOG(debug) << "adding static nodes";
				auto peersFile = boost::filesystem::path(m_pBootstrapper->resourcesPath()) / "peers-p2p.json";
				if (boost::filesystem::exists(peersFile))
					AddStaticNodesFromPath(*m_pBootstrapper, peersFile.generic_string());
				peersFile = boost::filesystem::path(m_pBootstrapper->resourcesPath()) / "peers-api.json";
				if (boost::filesystem::exists(peersFile))
					AddStaticNodesFromPath(*m_pBootstrapper, peersFile.generic_string());
				SeedNodeContainer(m_nodes, *m_pBootstrapper);

				CATAPULT_LOG(debug) << "booting extension services";
				auto& extensionManager = m_pBootstrapper->extensionManager();
				m_pServiceState = std::make_unique<extensions::ServiceState>(
						m_nodes,
						m_cacheHolder.cache(),
						m_catapultState,
						m_storage,
						m_score,
						*m_pUtCache,
						extensionManager.networkTimeSupplier(),
						*m_pTransactionStatusSubscriber,
						*m_pStateChangeSubscriber,
						*m_pNodeSubscriber,
						m_counters,
						m_pluginManager,
						m_pBootstrapper->pool());
				extensionManager.registerServices(m_serviceLocator, *m_pServiceState);
				for (const auto& counter : m_serviceLocator.counters())
					m_counters.push_back(counter);

				m_isBooted = true;

				// save nemesis state on first boot so that state directory is created and NemesisBlockNotifier
				// is always bypassed on subsequent boots
				if (isFirstBoot)
					saveStateToDisk();

			}

		private:
			void registerCounters() {
				AddMemoryCounters(m_counters);
				m_counters.emplace_back(utils::DiagnosticCounterId("TOT CONF TXES"), [&state = m_catapultState]() {
					return state.NumTotalTransactions.load();
				});

				m_pluginManager.addDiagnosticCounters(m_counters, m_cacheHolder.cache()); // add cache counters
				m_counters.emplace_back(utils::DiagnosticCounterId("UT CACHE"), [&source = *m_pUtCache]() {
					return source.view().size();
				});
			}

			bool executeAndNotifyNemesis() {

				NemesisBlockNotifier notifier(m_cacheHolder.cache(), m_storage, m_pluginManager);

				if (m_pBlockChangeSubscriber)
					notifier.raise(*m_pBlockChangeSubscriber);

				notifier.raise(*m_pStateChangeSubscriber);

				// skip next *two* messages because subscriber creates two files during raise (score change and state change)
				if (config().Node.ShouldEnableAutoSyncCleanup)
					io::FileQueueReader(m_dataDirectory.spoolDir("state_change").str(), "index_server_r.dat", "index_server.dat").skip(2);

				return true;
			}

			void loadStateFromDisk() {
				auto heights = extensions::LoadStateFromDirectory(m_dataDirectory.dir("state"), stateRef(), m_pluginManager);

				// if cache and storage heights are inconsistent, recovery is needed
				if (heights.Cache != heights.Storage) {
					std::ostringstream out;
					out << "cache height (" << heights.Cache << ") is inconsistent with storage height (" << heights.Storage << ")";
					CATAPULT_THROW_RUNTIME_ERROR(out.str().c_str());
				}

				CATAPULT_LOG(info) << "loaded block chain (height = " << heights.Cache << ", score = " << m_score.get() << ")";
			}

		public:
			void shutdown() override {
				utils::StackLogger stackLogger("shutting down local node", utils::LogLevel::Info);

				m_pBootstrapper->pool().shutdown();
				CATAPULT_CLEANUP_LOG(info, "Pool was shutdown, proceeding to save state");
				saveStateToDisk();
				m_pBootstrapper->configHolder()->SetPluginInitializer(nullptr);
				m_pBootstrapper->configHolder()->ClearPluginConfigurations();
				CATAPULT_CLEANUP_LOG(info, "Saved state to disk");
			}

		private:
			void saveStateToDisk() {
				// only save to storage if boot succeeded
				if (!m_isBooted)
					return;

				constexpr auto SaveStateToDirectoryWithCheckpointing = extensions::SaveStateToDirectoryWithCheckpointing;
				SaveStateToDirectoryWithCheckpointing(m_dataDirectory, config().Node, m_cacheHolder.cache(), m_catapultState, m_score.get());
			}

		public:
			const cache::CatapultCache& cache() const override {
				return m_cacheHolder.cache();
			}

			model::ChainScore score() const override {
				return m_score.get();
			}

			LocalNodeCounterValues counters() const override {
				LocalNodeCounterValues values;
				values.reserve(m_counters.size());
				for (const auto& counter : m_counters)
					values.emplace_back(counter);

				return values;
			}

			ionet::NodeContainerView nodes() const override {
				return m_nodes.view();
			}

		private:
			const config::BlockchainConfiguration& config() {
				return m_pluginManager.configHolder()->Config();
			}
			extensions::LocalNodeStateRef stateRef() {
				return extensions::LocalNodeStateRef(m_pluginManager.configHolder(), m_catapultState, m_cacheHolder.cache(), m_storage, m_score);
			}

		private:
			// make sure rooted services and modules are unloaded last
			std::vector<plugins::PluginModule> m_pluginModules;
			std::unique_ptr<extensions::ProcessBootstrapper> m_pBootstrapper;
			extensions::ServiceLocator m_serviceLocator;
			std::unique_ptr<extensions::ServiceState> m_pServiceState;

			io::BlockChangeSubscriber* m_pBlockChangeSubscriber;
			config::CatapultDataDirectory m_dataDirectory;
			ionet::NodeContainer m_nodes;

			extensions::CacheHolder& m_cacheHolder;
			state::CatapultState m_catapultState;
			io::BlockStorageCache m_storage;
			extensions::LocalNodeChainScore m_score;
			std::unique_ptr<cache::MemoryUtCacheProxy> m_pUtCache;

			std::unique_ptr<subscribers::TransactionStatusSubscriber> m_pTransactionStatusSubscriber;
			std::unique_ptr<subscribers::StateChangeSubscriber> m_pStateChangeSubscriber;
			std::unique_ptr<subscribers::NodeSubscriber> m_pNodeSubscriber;

			plugins::PluginManager& m_pluginManager;
			std::vector<utils::DiagnosticCounter> m_counters;
			bool m_isBooted;
		};
	}

	std::unique_ptr<LocalNode> CreateLocalNode(
			const crypto::KeyPair& keyPair,
			std::unique_ptr<extensions::ProcessBootstrapper>&& pBootstrapper) {
		return CreateAndBootHost<DefaultLocalNode>(std::move(pBootstrapper), keyPair);
	}
}}
