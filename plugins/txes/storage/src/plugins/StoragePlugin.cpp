/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/observers/Observers.h"
#include "StoragePlugin.h"
#include "src/config/StorageConfiguration.h"
#include "src/cache/DriveCache.h"
#include "src/cache/DriveCacheStorage.h"
#include "src/cache/DownloadCache.h"
#include "src/cache/DownloadCacheStorage.h"
#include "src/plugins/PrepareDriveTransactionPlugin.h"
#include "src/plugins/DataModificationTransactionPlugin.h"
#include "src/plugins/DownloadTransactionPlugin.h"
#include "src/plugins/DataModificationApprovalTransactionPlugin.h"
#include "src/validators/Validators.h"
#include "catapult/plugins/CacheHandlers.h"

namespace catapult { namespace plugins {

	void RegisterStorageSubsystem(PluginManager& manager) {

		manager.addPluginInitializer([](auto& config) {
			config.template InitPluginConfiguration<config::StorageConfiguration>();
		});


        const auto& pConfigHolder = manager.configHolder();
        const auto& immutableConfig = manager.immutableConfig();
		manager.addTransactionSupport(CreatePrepareDriveTransactionPlugin(immutableConfig));
		manager.addTransactionSupport(CreateDataModificationTransactionPlugin());
		manager.addTransactionSupport(CreateDownloadTransactionPlugin(immutableConfig));
		manager.addTransactionSupport(CreateDataModificationApprovalTransactionPlugin());


		manager.addCacheSupport<cache::DriveCacheStorage>(
			std::make_unique<cache::DriveCache>(manager.cacheConfig(cache::DriveCache::Name), pConfigHolder));

		using DriveCacheHandlersService = CacheHandlers<cache::DriveCacheDescriptor>;
		DriveCacheHandlersService::Register<model::FacilityCode::Drive>(manager);

		manager.addDiagnosticCounterHook([](auto& counters, const cache::CatapultCache& cache) {
			counters.emplace_back(utils::DiagnosticCounterId("DRIVE C"), [&cache]() {
				return cache.sub<cache::DriveCache>().createView(cache.height())->size();
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


		manager.addStatefulValidatorHook([](auto& builder) {
			builder
					.add(validators::CreateDataModificationApprovalValidator());
		});


		manager.addObserverHook([](auto& builder) {
			builder
					.add(observers::CreatePrepareDriveObserver())
					.add(observers::CreateDataModificationApprovalObserver());
		});
	}
}}


extern "C" PLUGIN_API
void RegisterSubsystem(catapult::plugins::PluginManager& manager) {
    catapult::plugins::RegisterStorageSubsystem(manager);
}
