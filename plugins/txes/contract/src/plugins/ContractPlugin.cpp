/**
*** Copyright 2018 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/plugins/PluginManager.h"
#include "ContractPlugin.h"
#include "src/cache/ContractCache.h"
#include "src/cache/ContractCacheStorage.h"
#include "src/cache/ReputationCache.h"
#include "src/cache/ReputationCacheStorage.h"
#include "src/observers/Observers.h"
#include "src/plugins/ModifyContractTransactionPlugin.h"
#include "src/validators/Validators.h"
#include "catapult/plugins/CacheHandlers.h"

namespace catapult { namespace plugins {

	void RegisterContractSubsystem(PluginManager& manager) {
		manager.addTransactionSupport(CreateModifyContractTransactionPlugin());

		manager.addCacheSupport<cache::ContractCacheStorage>(
			std::make_unique<cache::ContractCache>(manager.cacheConfig(cache::ContractCache::Name)));

		manager.addCacheSupport<cache::ReputationCacheStorage>(
			std::make_unique<cache::ReputationCache>(manager.cacheConfig(cache::ReputationCache::Name)));

		using CacheHandlersContract = CacheHandlers<cache::ContractCacheDescriptor>;
		CacheHandlersContract::Register<model::FacilityCode::Contract>(manager);

		manager.addDiagnosticCounterHook([](auto& counters, const cache::CatapultCache& cache) {
			counters.emplace_back(utils::DiagnosticCounterId("CONTRACT C"), [&cache]() {
				return cache.sub<cache::ContractCache>().createView(cache.height())->size();
			});
		});

		using CacheHandlersReputation = CacheHandlers<cache::ReputationCacheDescriptor>;
		CacheHandlersReputation::Register<model::FacilityCode::Reputation>(manager);

		manager.addDiagnosticCounterHook([](auto& counters, const cache::CatapultCache& cache) {
			counters.emplace_back(utils::DiagnosticCounterId("REPUTATION C"), [&cache]() {
				return cache.sub<cache::ReputationCache>().createView(cache.height())->size();
			});
		});

		manager.addStatelessValidatorHook([](auto& builder) {
			builder
					.add(validators::CreateModifyContractCustomersValidator())
					.add(validators::CreateModifyContractExecutorsValidator())
					.add(validators::CreateModifyContractVerifiersValidator())
					.add(validators::CreatePluginConfigValidator());
		});

		manager.addStatefulValidatorHook([](auto& builder) {
			builder
					.add(validators::CreateModifyContractInvalidCustomersValidator())
					.add(validators::CreateModifyContractInvalidExecutorsValidator())
					.add(validators::CreateModifyContractInvalidVerifiersValidator())
					.add(validators::CreateModifyContractDurationValidator());
		});

		manager.addObserverHook([pConfigHolder = manager.configHolder()](auto& builder) {
			builder.add(observers::CreateModifyContractObserver(pConfigHolder));
			builder.add(observers::CreateReputationUpdateObserver());
		});
	}
}}

extern "C" PLUGIN_API
void RegisterSubsystem(catapult::plugins::PluginManager& manager) {
	catapult::plugins::RegisterContractSubsystem(manager);
}
