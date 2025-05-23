/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/plugins/CacheHandlers.h"
#include "BlockchainUpgradePlugin.h"
#include "src/cache/BlockchainUpgradeCacheSubCachePlugin.h"
#include "src/config/BlockchainUpgradeConfiguration.h"
#include "src/observers/Observers.h"
#include "src/plugins/BlockchainUpgradeTransactionPlugin.h"
#include "src/validators/Validators.h"

namespace catapult { namespace plugins {

	void RegisterBlockchainUpgradeSubsystem(PluginManager& manager) {
		manager.addPluginInitializer([](auto& config) {
			config.template InitPluginConfiguration<config::BlockchainUpgradeConfiguration>();
		});

		manager.addTransactionSupport(CreateBlockchainUpgradeTransactionPlugin());

		manager.addCacheSupport(std::make_unique<cache::BlockchainUpgradeCacheSubCachePlugin>(manager.cacheConfig(cache::BlockchainUpgradeCache::Name), manager.configHolder()));

		using CacheHandlersBlockchainUpgrade = CacheHandlers<cache::BlockchainUpgradeCacheDescriptor>;
		CacheHandlersBlockchainUpgrade::Register<model::FacilityCode::BlockchainUpgrade>(manager);

		manager.addDiagnosticCounterHook([](auto& counters, const cache::CatapultCache& cache) {
			counters.emplace_back(utils::DiagnosticCounterId("UPGRADE C"), [&cache]() {
				return cache.sub<cache::BlockchainUpgradeCache>().createView(cache.height())->size();
			});
		});

		manager.addStatelessValidatorHook([](auto& builder) {
			builder
				.add(validators::CreateBlockchainUpgradePluginConfigValidator());
		});

		manager.addStatefulValidatorHook([](auto& builder) {
			builder
				.add(validators::CreateBlockchainUpgradeSignerValidator())
				.add(validators::CreateBlockchainUpgradeValidator())
				.add(validators::CreateBlockchainVersionValidator());
		});

		manager.addObserverHook([](auto& builder) {
			builder
				.add(observers::CreateBlockchainUpgradeObserver());
		});
	}
}}

extern "C" PLUGIN_API
void RegisterSubsystem(catapult::plugins::PluginManager& manager) {
	catapult::plugins::RegisterBlockchainUpgradeSubsystem(manager);
}
