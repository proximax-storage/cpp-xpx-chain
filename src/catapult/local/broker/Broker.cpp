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

#include "Broker.h"
#include "catapult/config/CatapultDataDirectory.h"
#include "catapult/extensions/ProcessBootstrapper.h"
#include "catapult/io/FileQueue.h"
#include "catapult/local/HostUtils.h"
#include "catapult/subscribers/BlockChangeReader.h"
#include "catapult/subscribers/BrokerMessageReaders.h"
#include "catapult/subscribers/PtChangeReader.h"
#include "catapult/subscribers/StateChangeReader.h"
#include "catapult/subscribers/TransactionStatusReader.h"
#include "catapult/subscribers/UtChangeReader.h"
#include "catapult/thread/Scheduler.h"
#include "catapult/utils/StackLogger.h"
#include "catapult/extensions/NemesisBlockLoader.h"
#include "catapult/config/ConfigurationFileLoader.h"
namespace catapult { namespace local {

	namespace {
		class DefaultBroker final : public Broker {
		public:
			explicit DefaultBroker(std::unique_ptr<extensions::ProcessBootstrapper>&& pBootstrapper)
					: m_pBootstrapper(std::move(pBootstrapper))
					, m_dataDirectory(config::CatapultDataDirectoryPreparer::Prepare(m_pBootstrapper->config().User.DataDirectory))
					, m_catapultCache({}) // note that sub caches are added in boot
					, m_pBlockChangeSubscriber(m_pBootstrapper->subscriptionManager().createBlockChangeSubscriber())
					, m_pUtChangeSubscriber(m_pBootstrapper->subscriptionManager().createUtChangeSubscriber())
					, m_pPtChangeSubscriber(m_pBootstrapper->subscriptionManager().createPtChangeSubscriber())
					, m_pTransactionStatusSubscriber(m_pBootstrapper->subscriptionManager().createTransactionStatusSubscriber())
					, m_pStateChangeSubscriber(m_pBootstrapper->subscriptionManager().createStateChangeSubscriber())
					, m_pluginManager(m_pBootstrapper->pluginManager())
			{}

			~DefaultBroker() override {
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
				io::FileBlockStorage storage(m_dataDirectory.rootDir().str(), io::FileBlockStorageMode::Hash_Index);
				//This is the first boot, so we must load the nemesis block network configuration
				const auto pNemesisBlockElement = storage.loadBlockElement(Height(1));
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

				CATAPULT_LOG(debug) << "initializing cache";
				m_catapultCache = m_pluginManager.createCache();
				m_pluginManager.configHolder()->SetCache(&m_catapultCache);

				utils::StackLogger stackLogger("booting broker", utils::LogLevel::Info);
				startIngestion();
			}

		public:
			void shutdown() override {
				utils::StackLogger stackLogger("shutting down broker", utils::LogLevel::Info);

				m_pBootstrapper->pool().shutdown();
			}

		private:
			void startIngestion() {
				using namespace catapult::subscribers;

				auto pServiceGroup = m_pBootstrapper->pool().pushServiceGroup("scheduler");
				auto pScheduler = pServiceGroup->pushService(thread::CreateScheduler);
				pScheduler->addTask(createIngestionTask("block_change", *m_pBlockChangeSubscriber, ReadNextBlockChange));
				pScheduler->addTask(createIngestionTask("unconfirmed_transactions_change", *m_pUtChangeSubscriber, ReadNextUtChange));
				pScheduler->addTask(createIngestionTask("partial_transactions_change", *m_pPtChangeSubscriber, ReadNextPtChange));
				pScheduler->addTask(createIngestionTask("transaction_status", *m_pTransactionStatusSubscriber, ReadNextTransactionStatus));
				pScheduler->addTask(createIngestionTask("state_change", *m_pStateChangeSubscriber, [&catapultCache = m_catapultCache](
						auto& inputStream,
						auto& subscriber) {
					return ReadNextStateChange(inputStream, catapultCache.changesStorages(), subscriber);
				}));
			}

			template<typename TSubscriber, typename TMessageReader>
			thread::Task createIngestionTask(const std::string& queueName, TSubscriber& subscriber, TMessageReader readNextMessage) {
				thread::Task task;
				task.StartDelay = utils::TimeSpan::FromMilliseconds(100);
				task.NextDelay = thread::CreateUniformDelayGenerator(utils::TimeSpan::FromMilliseconds(500));
				task.Name = queueName;

				auto queuePath = m_dataDirectory.spoolDir(queueName).str();
				task.Callback = [&subscriber, readNextMessage, queuePath]() {
					subscribers::ReadAll({ queuePath, "index_broker_r.dat", "index.dat" }, subscriber, readNextMessage);
					return thread::make_ready_future(thread::TaskResult::Continue);
				};

				return task;
			}

		private:
			// make sure modules are unloaded last
			std::vector<plugins::PluginModule> m_pluginModules;
			std::unique_ptr<extensions::ProcessBootstrapper> m_pBootstrapper;

			config::CatapultDataDirectory m_dataDirectory;

			cache::CatapultCache m_catapultCache;

			std::unique_ptr<io::BlockChangeSubscriber> m_pBlockChangeSubscriber;
			std::unique_ptr<cache::UtChangeSubscriber> m_pUtChangeSubscriber;
			std::unique_ptr<cache::PtChangeSubscriber> m_pPtChangeSubscriber;
			std::unique_ptr<subscribers::TransactionStatusSubscriber> m_pTransactionStatusSubscriber;
			std::unique_ptr<subscribers::StateChangeSubscriber> m_pStateChangeSubscriber;

			plugins::PluginManager& m_pluginManager;
		};
	}

	std::unique_ptr<Broker> CreateBroker(std::unique_ptr<extensions::ProcessBootstrapper>&& pBootstrapper) {
		return CreateAndBootHost<DefaultBroker>(std::move(pBootstrapper));
	}
}}
