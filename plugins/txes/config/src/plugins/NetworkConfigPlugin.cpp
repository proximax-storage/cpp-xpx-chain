/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/plugins/CacheHandlers.h"
#include "NetworkConfigPlugin.h"
#include "src/cache/NetworkConfigCacheSubCachePlugin.h"
#include "src/config/NetworkConfigConfiguration.h"
#include "src/observers/Observers.h"
#include "src/plugins/NetworkConfigTransactionPlugin.h"
#include "src/validators/Validators.h"

namespace catapult { namespace plugins {

	void RegisterNetworkConfigSubsystem(PluginManager& manager) {
		manager.addPluginInitializer([](auto& config) {
			config.template InitPluginConfiguration<config::NetworkConfigConfiguration>();
		});
		manager.addTransactionSupport(CreateNetworkConfigTransactionPlugin());

		const auto& pConfigHolder = manager.configHolder();
		manager.addCacheSupport(std::make_unique<cache::NetworkConfigCacheSubCachePlugin>(manager.cacheConfig(cache::NetworkConfigCache::Name), pConfigHolder));

		using CacheHandlersNetworkConfig = CacheHandlers<cache::NetworkConfigCacheDescriptor>;
		CacheHandlersNetworkConfig::Register<model::FacilityCode::NetworkConfig>(manager);

		manager.addDiagnosticCounterHook([](auto& counters, const cache::CatapultCache& cache) {
			counters.emplace_back(utils::DiagnosticCounterId("CONFIG C"), [&cache]() {
				return cache.sub<cache::NetworkConfigCache>().createView(cache.height())->size();
			});
		});

		manager.addStatelessValidatorHook([](auto& builder) {
			builder.add(validators::CreateNetworkConfigPluginConfigValidator());
		});

		manager.addStatefulValidatorHook([&manager](auto& builder) {
			builder
				.add(validators::CreateNetworkConfigSignerValidator())
				.add(validators::CreateNetworkConfigValidator(manager))
				.add(validators::CreatePluginAvailableValidator(manager));
		});

		manager.addObserverHook([](auto& builder) {
			builder
				.add(observers::CreateNetworkConfigObserver());
		});
	}
}}

extern "C" PLUGIN_API
void RegisterSubsystem(catapult::plugins::PluginManager& manager) {
	catapult::plugins::RegisterNetworkConfigSubsystem(manager);
}
