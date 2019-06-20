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

namespace catapult { namespace local {

	namespace {
		std::unique_ptr<io::PrunableBlockStorage> CreateStagingBlockStorage(const config::CatapultDataDirectory& dataDirectory) {
			auto stagingDirectory = dataDirectory.spoolDir("block_sync").str();
			boost::filesystem::create_directory(stagingDirectory);
			return std::make_unique<io::FileBlockStorage>(stagingDirectory, io::FileBlockStorageMode::None);
		}

		std::unique_ptr<subscribers::StateChangeSubscriber> CreateStateChangeSubscriber(
				subscribers::SubscriptionManager& subscriptionManager,
				const cache::CatapultCache& catapultCache,
				const config::CatapultDataDirectory& dataDirectory) {
			subscriptionManager.addStateChangeSubscriber(CreateFileStateChangeStorage(
					std::make_unique<io::FileQueueWriter>(dataDirectory.spoolDir("state_change").str(), "index_server.dat"),
					[&catapultCache]() { return catapultCache.changesStorages(); }));
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
					, m_config(m_pBootstrapper->config())
					, m_dataDirectory(config::CatapultDataDirectoryPreparer::Prepare(m_config.User.DataDirectory))
					, m_nodes(m_config.Node.MaxTrackedNodes, m_pBootstrapper->extensionManager().networkTimeSupplier())
					, m_catapultCache({}) // note that sub caches are added in boot
					, m_storage(
							m_pBootstrapper->subscriptionManager().createBlockStorage(m_pBlockChangeSubscriber),
							CreateStagingBlockStorage(m_dataDirectory))
					, m_pUtCache(m_pBootstrapper->subscriptionManager().createUtCache(extensions::GetUtCacheOptions(m_config.Node)))
					, m_pTransactionStatusSubscriber(m_pBootstrapper->subscriptionManager().createTransactionStatusSubscriber())
					, m_pStateChangeSubscriber(CreateStateChangeSubscriber(
							m_pBootstrapper->subscriptionManager(),
							m_catapultCache,
							m_dataDirectory))
					, m_pNodeSubscriber(CreateNodeSubscriber(m_pBootstrapper->subscriptionManager(), m_nodes))
					, m_pluginManager(m_pBootstrapper->pluginManager())
					, m_isBooted(false) {
				SeedNodeContainer(m_nodes, *m_pBootstrapper);
			}

			~DefaultLocalNode() override {
				shutdown();
			}

		public:
			void boot() {
				CATAPULT_LOG(info) << "registering system plugins";
				m_pluginModules = LoadAllPlugins(*m_pBootstrapper);

				CATAPULT_LOG(debug) << "initializing cache";
				m_catapultCache = m_pluginManager.createCache();

				CATAPULT_LOG(debug) << "registering counters";
				registerCounters();

				utils::StackLogger stackLogger("booting local node", utils::LogLevel::Info);
				auto isFirstBoot = executeAndNotifyNemesis();
				loadStateFromDisk();

				CATAPULT_LOG(debug) << "booting extension services";
				auto& extensionManager = m_pBootstrapper->extensionManager();
				auto serviceState = extensions::ServiceState(
						m_config,
						m_nodes,
						m_catapultCache,
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
				extensionManager.registerServices(m_serviceLocator, serviceState);
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
					return state.NumTotalTransactions;
				});

				m_pluginManager.addDiagnosticCounters(m_counters, m_catapultCache); // add cache counters
				m_counters.emplace_back(utils::DiagnosticCounterId("UT CACHE"), [&source = *m_pUtCache]() {
					return source.view().size();
				});
			}

			bool executeAndNotifyNemesis() {
				// only execute nemesis during first boot
				if (extensions::HasSerializedState(m_dataDirectory.dir("state")))
					return false;

				NemesisBlockNotifier notifier(m_config.BlockChain, m_catapultCache, m_storage, m_pluginManager);

				if (m_pBlockChangeSubscriber)
					notifier.raise(*m_pBlockChangeSubscriber);

				notifier.raise(*m_pStateChangeSubscriber);

				// skip next *two* messages because subscriber creates two files during raise (score change and state change)
				if (m_config.Node.ShouldEnableAutoSyncCleanup)
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
				saveStateToDisk();
			}

		private:
			void saveStateToDisk() {
				// only save to storage if boot succeeded
				if (!m_isBooted)
					return;

				constexpr auto SaveStateToDirectoryWithCheckpointing = extensions::SaveStateToDirectoryWithCheckpointing;
				SaveStateToDirectoryWithCheckpointing(m_dataDirectory, m_config.Node, m_catapultCache, m_catapultState, m_score.get());
			}

		public:
			const cache::CatapultCache& cache() const override {
				return m_catapultCache;
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
			extensions::LocalNodeStateRef stateRef() {
				return extensions::LocalNodeStateRef(m_config, m_catapultState, m_catapultCache, m_storage, m_score);
			}

		private:
			// make sure rooted services and modules are unloaded last
			std::vector<plugins::PluginModule> m_pluginModules;
			std::unique_ptr<extensions::ProcessBootstrapper> m_pBootstrapper;
			extensions::ServiceLocator m_serviceLocator;

			io::BlockChangeSubscriber* m_pBlockChangeSubscriber;
			const config::CatapultConfiguration& m_config;
			config::CatapultDataDirectory m_dataDirectory;
			ionet::NodeContainer m_nodes;

			cache::CatapultCache m_catapultCache;
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
