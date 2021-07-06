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

#include <catapult/ionet/PacketPayloadFactory.h>
#include "HarvestingService.h"
#include "HarvesterBlockGenerator.h"
#include "HarvestingUtFacadeFactory.h"
#include "ScheduledHarvesterTask.h"
#include "UnlockedAccounts.h"
#include "catapult/cache_core/ImportanceView.h"
#include "catapult/cache_tx/MemoryUtCache.h"
#include "catapult/crypto/KeyUtils.h"
#include "catapult/extensions/ConfigurationUtils.h"
#include "catapult/extensions/ExecutionConfigurationFactory.h"
#include "catapult/extensions/ServiceLocator.h"
#include "catapult/extensions/ServiceState.h"
#include "catapult/io/BlockStorageCache.h"
#include "catapult/plugins/PluginManager.h"
#include "UnlockedAccountsUpdater.h"

namespace catapult { namespace harvesting {

	namespace {

		struct UnlockedAccountsHolder {
			std::shared_ptr<UnlockedAccounts> pUnlockedAccounts;
			std::shared_ptr<UnlockedAccountsUpdater> pUnlockedAccountsUpdater;
		};

		std::shared_ptr<UnlockedAccounts> CreateUnlockedAccounts(const HarvestingConfiguration& config, const cache::CatapultCache& cache) {
			// unlock configured account if it's eligible to harvest the next block
			auto keyPair = crypto::KeyPair::FromString(config.HarvestKey);
			auto publicKey = keyPair.publicKey();
			auto pUnlockedAccounts = std::make_shared<UnlockedAccounts>(config.MaxUnlockedAccounts,
				CreateDelegatePrioritizer(config.DelegatePrioritizationPolicy, cache, publicKey));
			if (config.IsAutoHarvestingEnabled) {
				auto unlockResult = pUnlockedAccounts->modifier().add(std::move(keyPair));
				CATAPULT_LOG(info)
						<< "Unlocked harvesting account " << publicKey
						<< " for harvesting with result " << unlockResult;
			}

			return pUnlockedAccounts;
		}

		UnlockedAccountsHolder CreateUnlockedAccountsHolder(
				const HarvestingConfiguration& config,
				const extensions::ServiceState& state,
				const crypto::KeyPair& encryptionKeyPair) {
			auto pUnlockedAccounts = CreateUnlockedAccounts(config, state.cache());

			auto pUnlockedAccountsUpdater = std::make_shared<UnlockedAccountsUpdater>(
					state.cache(),
					*pUnlockedAccounts,
					crypto::KeyPair::FromString(config.HarvestKey).publicKey(),
					encryptionKeyPair,
					state.pluginManager().configHolder(),
					config::CatapultDataDirectory(state.config().User.DataDirectory));
			pUnlockedAccountsUpdater->load();

			return { pUnlockedAccounts, pUnlockedAccountsUpdater };
		}

		ScheduledHarvesterTaskOptions CreateHarvesterTaskOptions(extensions::ServiceState& state) {
			ScheduledHarvesterTaskOptions options;
			options.HarvestingAllowed = state.hooks().chainSyncedPredicate();
			options.LastBlockElementSupplier = [&storage = state.storage()]() {
				auto storageView = storage.view();
				return storageView.loadBlockElement(storageView.chainHeight());
			};
			options.TimeSupplier = state.timeSupplier();
			options.RangeConsumer = state.hooks().completionAwareBlockRangeConsumerFactory()(disruptor::InputSource::Local);
			return options;
		}
		/*
		void PruneUnlockedAccounts(UnlockedAccounts& unlockedAccounts, const cache::CatapultCache& cache,
				const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder) {
			auto cacheView = cache.createView();
			auto height = cacheView.height() + Height(1);
			auto readOnlyAccountStateCache = cache::ReadOnlyAccountStateCache(cacheView.sub<cache::AccountStateCache>());
			auto minHarvesterBalance = pConfigHolder->Config(height).Network.MinHarvesterBalance;
			unlockedAccounts.modifier().removeIf([height, minHarvesterBalance, &readOnlyAccountStateCache](const auto& key) {
				cache::ImportanceView view(readOnlyAccountStateCache);
				return !view.canHarvest(key, height, minHarvesterBalance);
			});
		}*/

		thread::Task CreateHarvestingTask(extensions::ServiceState& state, const UnlockedAccountsHolder& unlockedAccountsHolder, const Key& beneficiary) {
			const auto& cache = state.cache();
			const auto& pConfigHolder = state.pluginManager().configHolder();
			const auto& utCache = state.utCache();
			auto strategy = state.config().Node.TransactionSelectionStrategy;
			auto executionConfig = extensions::CreateExecutionConfiguration(state.pluginManager());
			HarvestingUtFacadeFactory utFacadeFactory(cache, executionConfig);
			auto pUnlockedAccounts = unlockedAccountsHolder.pUnlockedAccounts;
			auto pUnlockedAccountsUpdater = unlockedAccountsHolder.pUnlockedAccountsUpdater;
			auto blockGenerator = CreateHarvesterBlockGenerator(strategy, utFacadeFactory, utCache);
			auto pHarvesterTask = std::make_shared<ScheduledHarvesterTask>(
					CreateHarvesterTaskOptions(state),
					std::make_unique<Harvester>(cache, pConfigHolder, beneficiary, *pUnlockedAccounts, blockGenerator));

			return thread::CreateNamedTask("harvesting task", [&cache, pUnlockedAccountsUpdater, pHarvesterTask, state]() {
				// update unlocked accounts by pruning and filtering
			  	pUnlockedAccountsUpdater->update();

				// harvest the next block
				pHarvesterTask->harvest();
				return thread::make_ready_future(thread::TaskResult::Continue);
			});
		}
		// region diagnostic handler

		bool IsDiagnosticExtensionEnabled(const config::ExtensionsConfiguration& extensionsConfiguration) {
			const auto& names = extensionsConfiguration.Names;
			return names.cend() != std::find(names.cbegin(), names.cend(), "extension.diagnostics");
		}

		void RegisterDiagnosticUnlockedAccountsHandler(extensions::ServiceState& state, const UnlockedAccounts& unlockedAccounts) {
			auto& handlers = state.packetHandlers();

			handlers.registerHandler(ionet::PacketType::Unlocked_Accounts, [&unlockedAccounts](const auto& packet, auto& context) {
			  if (!ionet::IsPacketValid(packet, ionet::PacketType::Unlocked_Accounts))
				  return;

			  auto view = unlockedAccounts.view();
			  std::vector<Key> harvesterPublicKeys;
			  view.forEach([&harvesterPublicKeys](const auto& descriptor) {
				harvesterPublicKeys.push_back(descriptor.publicKey());
				return true;
			  });

			  const auto* pHarvesterPublicKeys = reinterpret_cast<const uint8_t*>(harvesterPublicKeys.data());
			  context.response(catapult::ionet::PacketPayloadFactory::FromFixedSizeRange(
					  ionet::PacketType::Unlocked_Accounts,
					  model::EntityRange<Key>::CopyFixed(pHarvesterPublicKeys, harvesterPublicKeys.size())));
			});

		}

		// endregion
		class HarvestingServiceRegistrar : public extensions::ServiceRegistrar {
		public:
			explicit HarvestingServiceRegistrar(const HarvestingConfiguration& config) : m_config(config)
			{}

			extensions::ServiceRegistrarInfo info() const override {
				return { "Harvesting", extensions::ServiceRegistrarPhase::Post_Range_Consumers };
			}

			void registerServiceCounters(extensions::ServiceLocator& locator) override {
				locator.registerServiceCounter<UnlockedAccounts>(
						"unlockedAccounts",
						"UNLKED ACCTS",
						[](const auto& accounts) { return accounts.view().size(); });
			}

			void registerServices(extensions::ServiceLocator& locator, extensions::ServiceState& state) override {

				auto unlockedAccountsHolder = CreateUnlockedAccountsHolder(m_config, state, locator.keyPair());
				locator.registerRootedService("unlockedAccounts", unlockedAccountsHolder.pUnlockedAccounts);

				// add tasks
				auto beneficiary = crypto::ParseKey(m_config.Beneficiary);
				state.tasks().push_back(CreateHarvestingTask(state, unlockedAccountsHolder, beneficiary));

				if (IsDiagnosticExtensionEnabled(state.config().Extensions))
					RegisterDiagnosticUnlockedAccountsHandler(state, *unlockedAccountsHolder.pUnlockedAccounts);
			}

		private:
			HarvestingConfiguration m_config;
		};
	}

	DECLARE_SERVICE_REGISTRAR(Harvesting)(const HarvestingConfiguration& config) {
		return std::make_unique<HarvestingServiceRegistrar>(config);
	}
}}
