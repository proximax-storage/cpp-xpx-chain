/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "StoragePlugin.h"
#include "src/cache/BcDriveCacheStorage.h"
#include "src/cache/DownloadChannelCacheStorage.h"
#include "src/cache/ReplicatorCacheSubCachePlugin.h"
#include "src/plugins/PrepareBcDriveTransactionPlugin.h"
#include "src/plugins/DataModificationTransactionPlugin.h"
#include "src/plugins/DownloadTransactionPlugin.h"
#include "src/plugins/DataModificationApprovalTransactionPlugin.h"
#include "src/plugins/DataModificationCancelTransactionPlugin.h"
#include "src/plugins/ReplicatorOnboardingTransactionPlugin.h"
#include "src/plugins/DriveClosureTransactionPlugin.h"
#include "src/state/CachedStorageState.h"
#include "src/validators/Validators.h"
#include "src/observers/Observers.h"
#include "catapult/plugins/CacheHandlers.h"

namespace catapult { namespace plugins {

	void RegisterStorageSubsystem(PluginManager& manager) {
		manager.addPluginInitializer([](auto& config) {
			config.template InitPluginConfiguration<config::StorageConfiguration>();
		});

		const auto& pConfigHolder = manager.configHolder();
		const auto& immutableConfig = manager.immutableConfig();
		manager.addTransactionSupport(CreatePrepareBcDriveTransactionPlugin(immutableConfig));
		manager.addTransactionSupport(CreateDataModificationTransactionPlugin(immutableConfig));
		manager.addTransactionSupport(CreateDownloadTransactionPlugin(immutableConfig));
		manager.addTransactionSupport(CreateDataModificationApprovalTransactionPlugin());
		manager.addTransactionSupport(CreateDataModificationCancelTransactionPlugin());
		manager.addTransactionSupport(CreateReplicatorOnboardingTransactionPlugin());
		manager.addTransactionSupport(CreateDriveClosureTransactionPlugin());

		manager.addCacheSupport<cache::BcDriveCacheStorage>(
			std::make_unique<cache::BcDriveCache>(manager.cacheConfig(cache::BcDriveCache::Name), pConfigHolder));

		using BcDriveCacheHandlersService = CacheHandlers<cache::BcDriveCacheDescriptor>;
		BcDriveCacheHandlersService::Register<model::FacilityCode::BcDrive>(manager);

		manager.addDiagnosticCounterHook([](auto& counters, const cache::CatapultCache& cache) {
			counters.emplace_back(utils::DiagnosticCounterId("BC DRIVE C"), [&cache]() {
				return cache.sub<cache::BcDriveCache>().createView(cache.height())->size();
			});
		});

		manager.addCacheSupport<cache::DownloadChannelCacheStorage>(
			std::make_unique<cache::DownloadChannelCache>(manager.cacheConfig(cache::DownloadChannelCache::Name), pConfigHolder));

		using DownloadChannelCacheHandlersService = CacheHandlers<cache::DownloadChannelCacheDescriptor>;
		DownloadChannelCacheHandlersService::Register<model::FacilityCode::DownloadChannel>(manager);

		manager.addDiagnosticCounterHook([](auto& counters, const cache::CatapultCache& cache) {
			counters.emplace_back(utils::DiagnosticCounterId("DOWNLOAD CH C"), [&cache]() {
				return cache.sub<cache::DownloadChannelCache>().createView(cache.height())->size();
			});
		});

		auto pKeyCollector = std::make_shared<cache::ReplicatorKeyCollector>();
		manager.addCacheSupport(std::make_unique<cache::ReplicatorCacheSubCachePlugin>(
			manager.cacheConfig(cache::ReplicatorCache::Name), pKeyCollector, pConfigHolder));

		using ReplicatorCacheHandlersService = CacheHandlers<cache::ReplicatorCacheDescriptor>;
		ReplicatorCacheHandlersService::Register<model::FacilityCode::Replicator>(manager);

		manager.addDiagnosticCounterHook([](auto& counters, const cache::CatapultCache& cache) {
			counters.emplace_back(utils::DiagnosticCounterId("REPLICATOR C"), [&cache]() {
				return cache.sub<cache::ReplicatorCache>().createView(cache.height())->size();
			});
		});

		manager.setStorageState(std::make_unique<state::CachedStorageState>(pKeyCollector));

		manager.addStatelessValidatorHook([](auto& builder) {
			builder
				.add(validators::CreateStoragePluginConfigValidator());
		});

		manager.addStatefulValidatorHook([pConfigHolder, &immutableConfig, pKeyCollector](auto& builder) {
		  	builder
				.add(validators::CreatePrepareDriveValidator(pKeyCollector))
				.add(validators::CreateDataModificationValidator())
				.add(validators::CreateDataModificationApprovalValidator())
				.add(validators::CreateDataModificationCancelValidator())
				.add(validators::CreateReplicatorOnboardingValidator())
				.add(validators::CreateDriveClosureValidator());
		});

		manager.addObserverHook([pKeyCollector](auto& builder) {
			builder
				.add(observers::CreatePrepareDriveObserver(pKeyCollector))
				.add(observers::CreateDownloadChannelObserver())
				.add(observers::CreateDataModificationObserver())
				.add(observers::CreateDataModificationApprovalObserver())
				.add(observers::CreateDataModificationCancelObserver())
				.add(observers::CreateReplicatorOnboardingObserver())
				.add(observers::CreateDriveClosureObserver());
		});
	}
}}


extern "C" PLUGIN_API
void RegisterSubsystem(catapult::plugins::PluginManager& manager) {
    catapult::plugins::RegisterStorageSubsystem(manager);
}
