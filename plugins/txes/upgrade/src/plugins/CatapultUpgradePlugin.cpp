/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/plugins/CacheHandlers.h"
#include "catapult/plugins/PluginManager.h"
#include "CatapultUpgradePlugin.h"
#include "src/cache/CatapultUpgradeCache.h"
#include "src/cache/CatapultUpgradeCacheStorage.h"
#include "src/observers/Observers.h"
#include "src/plugins/CatapultUpgradeTransactionPlugin.h"
#include "src/validators/Validators.h"

namespace catapult { namespace plugins {

	void RegisterCatapultUpgradeSubsystem(PluginManager& manager) {
		manager.addTransactionSupport(CreateCatapultUpgradeTransactionPlugin());

		manager.addCacheSupport<cache::CatapultUpgradeCacheStorage>(
			std::make_unique<cache::CatapultUpgradeCache>(manager.cacheConfig(cache::CatapultUpgradeCache::Name)));

		using CacheHandlersCatapultUpgrade = CacheHandlers<cache::CatapultUpgradeCacheDescriptor>;
		CacheHandlersCatapultUpgrade::Register<model::FacilityCode::CatapultUpgrade>(manager);

		manager.addDiagnosticCounterHook([](auto& counters, const cache::CatapultCache& cache) {
			counters.emplace_back(utils::DiagnosticCounterId("UPGRADE C"), [&cache]() {
				return cache.sub<cache::CatapultUpgradeCache>().createView(cache.height())->size();
			});
		});

		manager.addStatelessValidatorHook([](auto& builder) {
			builder
				.add(validators::CreatePluginConfigValidator());
		});

		const auto& pConfigHolder = manager.configHolder();
		manager.addStatefulValidatorHook([pConfigHolder](auto& builder) {
			builder
				.add(validators::CreateCatapultUpgradeSignerValidator())
				.add(validators::CreateCatapultUpgradeValidator(pConfigHolder))
				.add(validators::CreateCatapultVersionValidator());
		});

		manager.addObserverHook([](auto& builder) {
			builder
				.add(observers::CreateCatapultUpgradeObserver());
		});
	}
}}

extern "C" PLUGIN_API
void RegisterSubsystem(catapult::plugins::PluginManager& manager) {
	catapult::plugins::RegisterCatapultUpgradeSubsystem(manager);
}
