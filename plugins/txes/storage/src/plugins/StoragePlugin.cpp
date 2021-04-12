/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/observers/Observers.h"
#include "StoragePlugin.h"
#include "src/config/StorageConfiguration.h"
#include "src/cache/DownloadCache.h"
#include "src/cache/DownloadCacheStorage.h"
#include "src/plugins/DataModificationTransactionPlugin.h"
#include "src/plugins/DownloadTransactionPlugin.h"
#include "src/validators/Validators.h"
#include "catapult/plugins/CacheHandlers.h"

namespace catapult { namespace plugins {

	void RegisterStorageSubsystem(PluginManager& manager) {

		manager.addPluginInitializer([](auto& config) {
			config.template InitPluginConfiguration<config::StorageConfiguration>();
		});


        const auto& pConfigHolder = manager.configHolder();
        const auto& immutableConfig = manager.immutableConfig();
        manager.addTransactionSupport(CreateDataModificationTransactionPlugin());
		manager.addTransactionSupport(CreateDownloadTransactionPlugin(immutableConfig));

		manager.addCacheSupport<cache::DownloadCacheStorage>(
			std::make_unique<cache::DownloadCache>(manager.cacheConfig(cache::DownloadCache::Name), pConfigHolder));

		using DownloadCacheHandlersService = CacheHandlers<cache::DownloadCacheDescriptor>;
		DownloadCacheHandlersService::Register<model::FacilityCode::Download>(manager);

		manager.addDiagnosticCounterHook([](auto& counters, const cache::CatapultCache& cache) {
			counters.emplace_back(utils::DiagnosticCounterId("DRIVE C"), [&cache]() {
				return cache.sub<cache::DownloadCache>().createView(cache.height())->size();
			});
		});

		manager.addCacheSupport<cache::DownloadCacheStorage>(
			std::make_unique<cache::DownloadCache>(manager.cacheConfig(cache::DownloadCache::Name), pConfigHolder));

		using DownloadCacheHandlersService = CacheHandlers<cache::DownloadCacheDescriptor>;
		DownloadCacheHandlersService::Register<model::FacilityCode::Download>(manager);

		manager.addDiagnosticCounterHook([](auto& counters, const cache::CatapultCache& cache) {
			counters.emplace_back(utils::DiagnosticCounterId("DOWNLOAD CHANNEL C"), [&cache]() {
				return cache.sub<cache::DownloadCache>().createView(cache.height())->size();
			});
		});

		manager.addObserverHook([pConfigHolder, &immutableConfig](auto& builder) {
			builder
					.add(observers::CreateDownloadChannelObserver());
		});
	}
}}


extern "C" PLUGIN_API
void RegisterSubsystem(catapult::plugins::PluginManager& manager) {
    catapult::plugins::RegisterStorageSubsystem(manager);
}
