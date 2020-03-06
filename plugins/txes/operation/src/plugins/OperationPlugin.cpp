/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "OperationPlugin.h"
#include "src/cache/OperationCache.h"
#include "src/config/OperationConfiguration.h"
#include "src/model/OperationReceiptType.h"
#include "src/observers/Observers.h"
#include "src/plugins/OperationIdentifyTransactionPlugin.h"
#include "src/plugins/StartOperationTransactionPlugin.h"
#include "src/plugins/EndOperationTransactionPlugin.h"
#include "src/validators/Validators.h"
#include "catapult/observers/ObserverUtils.h"
#include "catapult/plugins/CacheHandlers.h"

namespace catapult { namespace plugins {

	void RegisterOperationSubsystem(PluginManager& manager) {
		manager.addPluginInitializer([](auto& config) {
			config.template InitPluginConfiguration<config::OperationConfiguration>();
		});

        const auto& immutableConfig = manager.immutableConfig();
        manager.addTransactionSupport(CreateOperationIdentifyTransactionPlugin());
        manager.addTransactionSupport(CreateStartOperationTransactionPlugin(immutableConfig));
        manager.addTransactionSupport(CreateEndOperationTransactionPlugin());

        const auto& pConfigHolder = manager.configHolder();
		manager.addCacheSupport<cache::OperationCacheStorage>(
				std::make_unique<cache::OperationCache>(manager.cacheConfig(cache::OperationCache::Name), pConfigHolder));

		using CacheHandlers = CacheHandlers<cache::OperationCacheDescriptor>;
		CacheHandlers::Register<model::FacilityCode::Operation>(manager);

		manager.addDiagnosticCounterHook([](auto& counters, const cache::CatapultCache& cache) {
			counters.emplace_back(utils::DiagnosticCounterId("OPERATION C"), [&cache]() {
				return cache.sub<cache::OperationCache>().createView(cache.height())->size();
			});
		});

		manager.addStatelessValidatorHook([](auto& builder) {
			builder
				.add(validators::CreateEmbeddedTransactionValidator())
				.add(validators::CreateOperationPluginConfigValidator())
				.add(validators::CreateStartOperationValidator())
				.add(validators::CreateOperationMosaicValidator());
		});

		manager.addStatefulValidatorHook([](auto& builder) {
			builder
				.add(validators::CreateOperationDurationValidator())
				.add(validators::CreateOperationIdentifyValidator())
				.add(validators::CreateEndOperationValidator());
		});

		manager.addObserverHook([](auto& builder) {
			builder
				.add(observers::CreateStartOperationObserver())
				.add(observers::CreateExpiredOperationObserver())
				.add(observers::CreateEndOperationObserver())
				.add(observers::CreateAggregateTransactionHashObserver())
				.add(observers::CreateCacheBlockTouchObserver<cache::OperationCache>("Operation", model::Receipt_Type_Operation_Expired));
				// Uncomment in case of the operation cache requires cleanup.
//				.add(observers::CreateCacheBlockPruningObserver<cache::OperationCache>("Operation", 1));
		});
	}
}}

extern "C" PLUGIN_API
void RegisterSubsystem(catapult::plugins::PluginManager& manager) {
	catapult::plugins::RegisterOperationSubsystem(manager);
}
