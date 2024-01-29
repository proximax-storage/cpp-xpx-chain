/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "DbrbPlugin.h"
#include "src/cache/DbrbViewCache.h"
#include "src/cache/DbrbViewCacheSubCachePlugin.h"
#include "src/cache/DbrbViewFetcherImpl.h"
#include "src/model/AddDbrbProcessTransaction.h"
#include "src/observers/Observers.h"
#include "src/plugins/AddDbrbProcessTransactionPlugin.h"
#include "src/plugins/RemoveDbrbProcessTransactionPlugin.h"
#include "src/validators/Validators.h"
#include "catapult/plugins/CacheHandlers.h"

namespace catapult { namespace plugins {

	void RegisterDbrbSubsystem(PluginManager& manager) {
		manager.addPluginInitializer([](auto& config) {
			config.template InitPluginConfiguration<config::DbrbConfiguration>();
		});

		const auto& pConfigHolder = manager.configHolder();
		const auto& immutableConfig = manager.immutableConfig();
		manager.addTransactionSupport(CreateAddDbrbProcessTransactionPlugin());
		manager.addTransactionSupport(CreateRemoveDbrbProcessTransactionPlugin());

		auto pDbrbViewFetcher = std::make_shared<cache::DbrbViewFetcherImpl>();
		manager.addCacheSupport(std::make_unique<cache::DbrbViewCacheSubCachePlugin>(manager.cacheConfig(cache::DbrbViewCache::Name), pConfigHolder, pDbrbViewFetcher));

		using DbrbViewCacheHandlersService = CacheHandlers<cache::DbrbViewCacheDescriptor>;
		DbrbViewCacheHandlersService::Register<model::FacilityCode::DbrbView>(manager);

		manager.addDiagnosticCounterHook([](auto& counters, const cache::CatapultCache& cache) {
			counters.emplace_back(utils::DiagnosticCounterId("DBRB VIEW C"), [&cache]() {
				return cache.sub<cache::DbrbViewCache>().createView(cache.height())->size();
			});
		});

		manager.setDbrbViewFetcher(pDbrbViewFetcher);

		auto pTransactionFeeCalculator = manager.transactionFeeCalculator();
		pTransactionFeeCalculator->addUnlimitedFeeTransaction(model::AddDbrbProcessTransaction::Entity_Type, model::AddDbrbProcessTransaction::Current_Version);

		manager.addStatefulValidatorHook([pConfigHolder, &immutableConfig](auto& builder) {
		  	builder
				.add(validators::CreateAddDbrbProcessValidator());
		});

		manager.addObserverHook([](auto& builder) {
			builder
				.add(observers::CreateAddDbrbProcessObserver())
				.add(observers::CreateDbrbProcessPruningObserver())
				.add(observers::CreateRemoveDbrbProcessObserver());
		});
	}
}}


extern "C" PLUGIN_API
void RegisterSubsystem(catapult::plugins::PluginManager& manager) {
    catapult::plugins::RegisterDbrbSubsystem(manager);
}
