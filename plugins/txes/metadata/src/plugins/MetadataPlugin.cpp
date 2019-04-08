/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "MetadataPlugin.h"
#include "src/config/MetadataConfiguration.h"
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
		auto config = model::LoadPluginConfiguration<config::MetadataConfiguration>(manager.config(), "catapult.plugins.metadata");

		manager.addCacheSupport<cache::MetadataCacheStorage>(
				std::make_unique<cache::MetadataCache>(manager.cacheConfig(cache::MetadataCache::Name)));

		using CacheHandlers = CacheHandlers<cache::MetadataCacheDescriptor>;
		CacheHandlers::Register<model::FacilityCode::Metadata>(manager);

		manager.addDiagnosticCounterHook([](auto& counters, const cache::CatapultCache& cache) {
			counters.emplace_back(utils::DiagnosticCounterId("METADATA C"), [&cache]() {
				return cache.sub<cache::MetadataCache>().createView()->size();
			});
		});

		manager.addStatelessValidatorHook([&config](auto& builder) {
			builder
				.add(validators::CreateMetadataFieldModificationValidator(config.MaxFieldKeySize, config.MaxFieldValueSize));
//				.add(validators::CreatePropertyAddressNoSelfModificationValidator(networkIdentifier))
//				.add(validators::CreateMosaicPropertyModificationTypesValidator())
//				.add(validators::CreateTransactionTypePropertyModificationTypesValidator())
//				.add(validators::CreateTransactionTypePropertyModificationValuesValidator());
		});
//
//		auto config = model::LoadPluginConfiguration<config::PropertyConfiguration>(manager.config(), "catapult.plugins.property");
//		manager.addStatefulValidatorHook([maxPropertyValues = config.MaxPropertyValues](auto& builder) {
//			builder
//				.add(validators::CreateAddressPropertyRedundantModificationValidator())
//				.add(validators::CreateAddressPropertyValueModificationValidator())
//				.add(validators::CreateMaxAddressPropertyValuesValidator(maxPropertyValues))
//				.add(validators::CreateAddressInteractionValidator())
//				.add(validators::CreateMosaicPropertyRedundantModificationValidator())
//				.add(validators::CreateMosaicPropertyValueModificationValidator())
//				.add(validators::CreateMaxMosaicPropertyValuesValidator(maxPropertyValues))
//				.add(validators::CreateMosaicRecipientValidator())
//				.add(validators::CreateTransactionTypePropertyRedundantModificationValidator())
//				.add(validators::CreateTransactionTypePropertyValueModificationValidator())
//				.add(validators::CreateMaxTransactionTypePropertyValuesValidator(maxPropertyValues))
//				.add(validators::CreateTransactionTypeValidator())
//				.add(validators::CreateTransactionTypeNoSelfBlockingValidator());
//		});
//
		manager.addObserverHook([](auto& builder) {
			builder
				.add(observers::CreateAddressMetadataValueModificationObserver())
				.add(observers::CreateMosaicMetadataValueModificationObserver())
				.add(observers::CreateNamespaceMetadataValueModificationObserver());
		});

		auto maxRollbackBlocks = BlockDuration(manager.config().MaxRollbackBlocks);
		manager.addObserverHook([maxRollbackBlocks](auto& builder) {
			builder
					.add(observers::CreateCacheBlockPruningObserver<cache::MetadataCache>("Metadata", 1, maxRollbackBlocks));
		});
	}
}}

extern "C" PLUGIN_API
void RegisterSubsystem(catapult::plugins::PluginManager& manager) {
	catapult::plugins::RegisterMetadataSubsystem(manager);
}
