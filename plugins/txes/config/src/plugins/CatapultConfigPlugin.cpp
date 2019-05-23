/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/plugins/CacheHandlers.h"
#include "catapult/plugins/PluginManager.h"
#include "CatapultConfigPlugin.h"
#include "src/cache/CatapultConfigCache.h"
#include "src/cache/CatapultConfigCacheStorage.h"
#include "src/config/CatapultConfigConfiguration.h"
#include "src/observers/Observers.h"
#include "src/plugins/CatapultConfigTransactionPlugin.h"
#include "src/validators/Validators.h"

namespace catapult { namespace plugins {

	void RegisterCatapultConfigSubsystem(PluginManager& manager) {
		manager.addTransactionSupport(CreateCatapultConfigTransactionPlugin());
		auto config = model::LoadPluginConfiguration<config::CatapultConfigConfiguration>(manager.config(), "catapult.plugins.config");

		manager.addCacheSupport<cache::CatapultConfigCacheStorage>(
			std::make_unique<cache::CatapultConfigCache>(manager.cacheConfig(cache::CatapultConfigCache::Name)));

		using CacheHandlersCatapultConfig = CacheHandlers<cache::CatapultConfigCacheDescriptor>;
		CacheHandlersCatapultConfig::Register<model::FacilityCode::CatapultConfig>(manager);

		manager.addDiagnosticCounterHook([](auto& counters, const cache::CatapultCache& cache) {
			counters.emplace_back(utils::DiagnosticCounterId("CONFIG C"), [&cache]() {
				return cache.sub<cache::CatapultConfigCache>().createView()->size();
			});
		});

		manager.addStatefulValidatorHook([config](auto& builder) {
			builder
				.add(validators::CreateCatapultConfigSignerValidator())
				.add(validators::CreateCatapultConfigValidator(config.MaxBlockChainConfigSize.bytes32()));
		});

		manager.addObserverHook([](auto& builder) {
			builder
				.add(observers::CreateCatapultConfigObserver());
		});
	}
}}

extern "C" PLUGIN_API
void RegisterSubsystem(catapult::plugins::PluginManager& manager) {
	catapult::plugins::RegisterCatapultConfigSubsystem(manager);
}
