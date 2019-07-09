/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "MetadataPlugin.h"
#include "src/cache/MetadataCache.h"
#include "src/cache/MetadataCacheStorage.h"
#include "src/plugins/MetadataTransactionPlugin.h"
#include "src/observers/Observers.h"
#include "src/validators/Validators.h"
#include "catapult/observers/ObserverUtils.h"
#include "catapult/plugins/CacheHandlers.h"
#include "catapult/plugins/PluginManager.h"

namespace catapult { namespace plugins {

	void RegisterMetadataSubsystem(PluginManager& manager) {
		manager.addTransactionSupport(CreateAddressMetadataTransactionPlugin());
		manager.addTransactionSupport(CreateMosaicMetadataTransactionPlugin());
		manager.addTransactionSupport(CreateNamespaceMetadataTransactionPlugin());

		manager.addCacheSupport<cache::MetadataCacheStorage>(
			std::make_unique<cache::MetadataCache>(manager.cacheConfig(cache::MetadataCache::Name)));

		using CacheHandlers = CacheHandlers<cache::MetadataCacheDescriptor>;
		CacheHandlers::Register<model::FacilityCode::Metadata>(manager);

		manager.addDiagnosticCounterHook([](auto& counters, const cache::CatapultCache& cache) {
			counters.emplace_back(utils::DiagnosticCounterId("METADATA C"), [&cache]() {
				return cache.sub<cache::MetadataCache>().createView(cache.height())->size();
			});
		});

		manager.addStatelessValidatorHook([](auto& builder) {
			builder
				.add(validators::CreateMetadataTypeValidator());
		});

		const auto& pConfigHolder = manager.configHolder();
		manager.addStatefulValidatorHook([pConfigHolder](auto& builder) {
			builder
				.add(validators::CreateMetadataFieldModificationValidator(pConfigHolder))
				.add(validators::CreateModifyAddressMetadataValidator())
				.add(validators::CreateModifyMosaicMetadataValidator())
				.add(validators::CreateModifyNamespaceMetadataValidator())
				.add(validators::CreateMetadataModificationsValidator(pConfigHolder));
		});

		manager.addObserverHook([](auto& builder) {
			builder
				.add(observers::CreateAddressMetadataValueModificationObserver())
				.add(observers::CreateMosaicMetadataValueModificationObserver())
				.add(observers::CreateNamespaceMetadataValueModificationObserver());
		});

		manager.addObserverHook([pConfigHolder](auto& builder) {
			builder
				.add(observers::CreateCacheBlockPruningObserver<cache::MetadataCache>("Metadata", 1, pConfigHolder));
		});
	}
}}

extern "C" PLUGIN_API
void RegisterSubsystem(catapult::plugins::PluginManager& manager) {
	catapult::plugins::RegisterMetadataSubsystem(manager);
}
