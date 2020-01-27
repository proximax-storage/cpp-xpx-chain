/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/plugins/PluginManager.h"
#include "SuperContractPlugin.h"
#include "src/cache/SuperContractCache.h"
#include "src/cache/MongoSuperContractCacheStorage.h"
#include "src/model/SuperContractNotifications.h"
#include "src/observers/Observers.h"
#include "src/plugins/DeployTransactionPlugin.h"
#include "src/plugins/ExecuteTransactionPlugin.h"
#include "src/validators/Validators.h"
#include "catapult/plugins/CacheHandlers.h"
#include "src/config/SuperContractConfiguration.h"

namespace catapult { namespace plugins {

	void RegisterSuperContractSubsystem(PluginManager& manager) {

		manager.addPluginInitializer([](auto& config) {
			config.template InitPluginConfiguration<config::SuperContractConfiguration>();
		});

        const auto& pConfigHolder = manager.configHolder();
        manager.addTransactionSupport(CreateDeployTransactionPlugin());
        manager.addTransactionSupport(CreateExecuteTransactionPlugin());

		manager.addCacheSupport<cache::SuperContractCacheStorage>(
			std::make_unique<cache::SuperContractCache>(manager.cacheConfig(cache::SuperContractCache::Name), pConfigHolder));

		using SuperContractCacheHandlersSuperContract = CacheHandlers<cache::SuperContractCacheDescriptor>;
		SuperContractCacheHandlersSuperContract::Register<model::FacilityCode::Drive>(manager);

		manager.addDiagnosticCounterHook([](auto& counters, const cache::CatapultCache& cache) {
			counters.emplace_back(utils::DiagnosticCounterId("DRIVE C"), [&cache]() {
				return cache.sub<cache::SuperContractCache>().createView(cache.height())->size();
			});
		});

		manager.addStatefulValidatorHook([](auto& builder) {
			builder
					.add(validators::CreateDriveValidator())
					.add(validators::CreateDeployValidator())
					.add(validators::CreateDriveFileSystemValidator());
		});

		manager.addObserverHook([](auto& builder) {
			builder
					.add(observers::CreateDeployObserver());
		});
	}
}}


extern "C" PLUGIN_API
void RegisterSubsystem(catapult::plugins::PluginManager& manager) {
    catapult::plugins::RegisterSuperContractSubsystem(manager);
}
