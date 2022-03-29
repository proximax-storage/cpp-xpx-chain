/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <src/cache/LiquidityProviderCacheSubCachePlugin.h>
#include "CreateLiquidityProviderTransactionPlugin.h"
#include "src/validators/Validators.h"
#include "src/observers/Observers.h"
#include "src/config/LiquidityProviderConfiguration.h"
#include "catapult/plugins/CacheHandlers.h"

namespace catapult { namespace plugins {

	void RegisterStorageSubsystem(PluginManager& manager) {
		manager.addPluginInitializer([](auto& config) {
			config.template InitPluginConfiguration<config::LiquidityProviderConfiguration>();
		});

		const auto& pConfigHolder = manager.configHolder();
		const auto& immutableConfig = manager.immutableConfig();

		manager.addTransactionSupport(CreateCreateLiquidityProviderTransactionPlugin(immutableConfig));

		auto pKeyCollector = std::make_shared<cache::LiquidityProviderKeyCollector>();
		manager.addCacheSupport(std::make_unique<cache::LiquidityProviderCacheSubCachePlugin>(
			manager.cacheConfig(cache::LiquidityProviderCache::Name), pKeyCollector, pConfigHolder));

		using LiquidityProviderCacheHandlersService = CacheHandlers<cache::LiquidityProviderCacheDescriptor>;
		LiquidityProviderCacheHandlersService::Register<model::FacilityCode::LiquidityProvider>(manager);

		manager.addDiagnosticCounterHook([](auto& counters, const cache::CatapultCache& cache) {
			counters.emplace_back(utils::DiagnosticCounterId("LP C"), [&cache]() {
				return cache.sub<cache::LiquidityProviderCache>().createView(cache.height())->size();
			});
		});

		manager.addStatefulValidatorHook([pConfigHolder, &immutableConfig](auto& builder) {
//		  	builder
		});

		manager.addObserverHook([pKeyCollector] (auto& builder) {
			builder
			.add(observers::CreateSlashingObserver(pKeyCollector))
			.add(observers::CreateSlashingObserver(pKeyCollector));
		});
	}
}}


extern "C" PLUGIN_API
void RegisterSubsystem(catapult::plugins::PluginManager& manager) {
    catapult::plugins::RegisterStorageSubsystem(manager);
}
