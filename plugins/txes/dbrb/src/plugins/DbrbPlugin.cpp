/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "DbrbPlugin.h"
#include "src/cache/ViewSequenceCache.h"
#include "src/cache/ViewSequenceCacheStorage.h"
#include "src/model/InstallMessageTransaction.h"
#include "src/observers/Observers.h"
#include "src/plugins/InstallMessageTransactionPlugin.h"
#include "src/state/DbrbViewFetcherImpl.h"
#include "src/validators/Validators.h"
#include "catapult/plugins/CacheHandlers.h"

namespace catapult { namespace plugins {

	void RegisterDbrbSubsystem(PluginManager& manager) {
		manager.addPluginInitializer([](auto& config) {
			config.template InitPluginConfiguration<config::DbrbConfiguration>();
		});

		const auto& pConfigHolder = manager.configHolder();
		const auto& immutableConfig = manager.immutableConfig();
		manager.addTransactionSupport(CreateInstallMessageTransactionPlugin());

		manager.addCacheSupport<cache::ViewSequenceCacheStorage>(
			std::make_unique<cache::ViewSequenceCache>(manager.cacheConfig(cache::ViewSequenceCache::Name), pConfigHolder));

		using ViewSequenceCacheHandlersService = CacheHandlers<cache::ViewSequenceCacheDescriptor>;
		ViewSequenceCacheHandlersService::Register<model::FacilityCode::ViewSequence>(manager);

		manager.addDiagnosticCounterHook([](auto& counters, const cache::CatapultCache& cache) {
			counters.emplace_back(utils::DiagnosticCounterId("VIEW SEQ C"), [&cache]() {
				return cache.sub<cache::ViewSequenceCache>().createView(cache.height())->size();
			});
		});

		manager.setDbrbViewFetcher(std::make_shared<state::DbrbViewFetcherImpl>());

		auto pTransactionFeeCalculator = manager.transactionFeeCalculator();
		pTransactionFeeCalculator->addUnlimitedFeeTransaction(model::InstallMessageTransaction::Entity_Type, model::InstallMessageTransaction::Current_Version);

		manager.addStatefulValidatorHook([pConfigHolder, &immutableConfig](auto& builder) {
		  	builder
				.add(validators::CreateInstallMessageValidator());
		});

		manager.addObserverHook([](auto& builder) {
			builder
				.add(observers::CreateInstallMessageObserver());
		});
	}
}}


extern "C" PLUGIN_API
void RegisterSubsystem(catapult::plugins::PluginManager& manager) {
    catapult::plugins::RegisterDbrbSubsystem(manager);
}
