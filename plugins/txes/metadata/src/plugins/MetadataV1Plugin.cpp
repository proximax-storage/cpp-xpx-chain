/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "MetadataV1Plugin.h"
#include "src/cache/MetadataV1Cache.h"
#include "src/plugins/MetadataV1TransactionPlugin.h"
#include "src/observers/Observers.h"
#include "src/validators/Validators.h"
#include "catapult/observers/ObserverUtils.h"
#include "catapult/plugins/CacheHandlers.h"

namespace catapult { namespace plugins {

	void RegisterMetadataV1Subsystem(PluginManager& manager) {
		manager.addPluginInitializer([](auto& config) {
			config.template InitPluginConfiguration<config::MetadataV1Configuration>();
		});
		manager.addTransactionSupport(CreateAddressMetadataV1TransactionPlugin());
		manager.addTransactionSupport(CreateMosaicMetadataV1TransactionPlugin());
		manager.addTransactionSupport(CreateNamespaceMetadataV1TransactionPlugin());

		manager.addCacheSupport<cache::MetadataV1CacheStorage>(
			std::make_unique<cache::MetadataV1Cache>(manager.cacheConfig(cache::MetadataV1Cache::Name)));

		using CacheHandlers = CacheHandlers<cache::MetadataV1CacheDescriptor>;
		CacheHandlers::Register<model::FacilityCode::Metadata>(manager);

		manager.addDiagnosticCounterHook([](auto& counters, const cache::CatapultCache& cache) {
			counters.emplace_back(utils::DiagnosticCounterId("METADATA C"), [&cache]() {
				return cache.sub<cache::MetadataV1Cache>().createView(cache.height())->size();
			});
		});

		manager.addStatelessValidatorHook([](auto& builder) {
			builder
				.add(validators::CreateMetadataV1TypeValidator())
				.add(validators::CreateMetadataV1PluginConfigValidator());
		});

		manager.addStatefulValidatorHook([](auto& builder) {
			builder
				.add(validators::CreateMetadataV1FieldModificationValidator())
				.add(validators::CreateModifyAddressMetadataV1Validator())
				.add(validators::CreateModifyMosaicMetadataV1Validator())
				.add(validators::CreateModifyNamespaceMetadataV1Validator())
				.add(validators::CreateMetadataV1ModificationsValidator());
		});

		manager.addObserverHook([](auto& builder) {
			builder
				.add(observers::CreateAddressMetadataV1ValueModificationObserver())
				.add(observers::CreateMosaicMetadataV1ValueModificationObserver())
				.add(observers::CreateNamespaceMetadataV1ValueModificationObserver());
		});

		manager.addObserverHook([](auto& builder) {
			builder
				.add(observers::CreateCacheBlockPruningObserver<cache::MetadataV1Cache>("Metadata", 1));
		});
	}
}}

extern "C" PLUGIN_API
void RegisterSubsystem(catapult::plugins::PluginManager& manager) {
	catapult::plugins::RegisterMetadataV1Subsystem(manager);
}
