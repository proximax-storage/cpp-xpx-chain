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
#include "src/model/AddOrUpdateDbrbProcessTransaction.h"
#include "src/model/RemoveDbrbProcessTransaction.h"
#include "src/model/RemoveDbrbProcessByNetworkTransaction.h"
#include "src/observers/Observers.h"
#include "src/plugins/AddDbrbProcessTransactionPlugin.h"
#include "src/plugins/AddOrUpdateDbrbProcessTransactionPlugin.h"
#include "src/plugins/RemoveDbrbProcessTransactionPlugin.h"
#include "src/plugins/RemoveDbrbProcessByNetworkTransactionPlugin.h"
#include "src/validators/Validators.h"
#include "catapult/plugins/CacheHandlers.h"

namespace catapult { namespace plugins {

	void RegisterDbrbSubsystem(PluginManager& manager) {
		manager.addPluginInitializer([](auto& config) {
			config.template InitPluginConfiguration<config::DbrbConfiguration>();
		});

		manager.addTransactionSupport(CreateAddDbrbProcessTransactionPlugin());
		manager.addTransactionSupport(CreateAddOrUpdateDbrbProcessTransactionPlugin());
		manager.addTransactionSupport(CreateRemoveDbrbProcessTransactionPlugin());
		manager.addTransactionSupport(CreateRemoveDbrbProcessByNetworkTransactionPlugin());

		auto pDbrbViewFetcher = std::make_shared<cache::DbrbViewFetcherImpl>();
		manager.addCacheSupport(std::make_unique<cache::DbrbViewCacheSubCachePlugin>(manager.cacheConfig(cache::DbrbViewCache::Name), manager.configHolder(), pDbrbViewFetcher));

		using DbrbViewCacheHandlersService = CacheHandlers<cache::DbrbViewCacheDescriptor>;
		DbrbViewCacheHandlersService::Register<model::FacilityCode::DbrbView>(manager);

		manager.addDiagnosticCounterHook([](auto& counters, const cache::CatapultCache& cache) {
			counters.emplace_back(utils::DiagnosticCounterId("DBRB VIEW C"), [&cache]() {
				return cache.sub<cache::DbrbViewCache>().createView(cache.height())->size();
			});
		});

		manager.setDbrbViewFetcher(pDbrbViewFetcher);

		auto pTransactionFeeCalculator = manager.transactionFeeCalculator();
		for (auto version = 1u; version <= model::AddDbrbProcessTransaction::Current_Version; ++version)
			pTransactionFeeCalculator->addLimitedFeeTransaction(model::AddDbrbProcessTransaction::Entity_Type, version);
		for (auto version = 1u; version <= model::RemoveDbrbProcessTransaction::Current_Version; ++version)
			pTransactionFeeCalculator->addLimitedFeeTransaction(model::AddDbrbProcessTransaction::Entity_Type, version);
		for (auto version = 1u; version <= model::RemoveDbrbProcessByNetworkTransaction::Current_Version; ++version)
			pTransactionFeeCalculator->addLimitedFeeTransaction(model::RemoveDbrbProcessByNetworkTransaction::Entity_Type, version);
		for (auto version = 1u; version <= model::AddOrUpdateDbrbProcessTransaction::Current_Version; ++version)
			pTransactionFeeCalculator->addLimitedFeeTransaction(model::AddOrUpdateDbrbProcessTransaction::Entity_Type, version);

		manager.addStatefulValidatorHook([&dbrbViewFetcher = *pDbrbViewFetcher](auto& builder) {
		  	builder
				.add(validators::CreateAddDbrbProcessValidator())
				.add(validators::CreateRemoveDbrbProcessByNetworkValidator(dbrbViewFetcher))
				.add(validators::CreateNodeBootKeyValidator());
		});

		manager.addObserverHook([&manager](auto& builder) {
			builder
				.add(observers::CreateAddDbrbProcessObserver())
				.add(observers::CreateDbrbProcessPruningObserver(manager.dbrbProcessUpdateListeners()))
				.add(observers::CreateDbrbProcessUpdateObserver(manager))
				.add(observers::CreateRemoveDbrbProcessObserver());
		});
	}
}}


extern "C" PLUGIN_API
void RegisterSubsystem(catapult::plugins::PluginManager& manager) {
    catapult::plugins::RegisterDbrbSubsystem(manager);
}
