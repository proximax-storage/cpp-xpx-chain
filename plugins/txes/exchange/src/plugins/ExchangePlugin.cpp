/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <plugins/txes/exchange/src/config/ExchangeConfiguration.h>
#include "catapult/plugins/CacheHandlers.h"
#include "catapult/plugins/PluginManager.h"
#include "ExchangePlugin.h"
#include "src/cache/ExchangeCache.h"
#include "src/cache/ExchangeCacheStorage.h"
#include "src/observers/Observers.h"
#include "src/plugins/ExchangeOfferTransactionPlugin.h"
#include "src/plugins/RemoveExchangeOfferTransactionPlugin.h"
#include "src/plugins/ExchangeTransactionPlugin.h"
#include "src/validators/Validators.h"

namespace catapult { namespace plugins {

	void RegisterExchangeSubsystem(PluginManager& manager) {

		manager.addPluginInitializer([](auto& config) {
			config.template InitPluginConfiguration<config::ExchangeConfiguration>();
		});

		const auto& immutableConfig = manager.immutableConfig();
		manager.addTransactionSupport(CreateExchangeOfferTransactionPlugin(immutableConfig));
		manager.addTransactionSupport(CreateExchangeTransactionPlugin(immutableConfig));
		manager.addTransactionSupport(CreateRemoveExchangeOfferTransactionPlugin());

		auto pConfigHolder = manager.configHolder();
		manager.addCacheSupport<cache::ExchangeCacheStorage>(
			std::make_unique<cache::ExchangeCache>(manager.cacheConfig(cache::ExchangeCache::Name), pConfigHolder));

		using CacheHandlersOffer = CacheHandlers<cache::ExchangeCacheDescriptor>;
		CacheHandlersOffer::Register<model::FacilityCode::Exchange>(manager);

		manager.addDiagnosticCounterHook([](auto& counters, const cache::CatapultCache& cache) {
			counters.emplace_back(utils::DiagnosticCounterId("EXCHANGE C"), [&cache]() {
				return cache.sub<cache::ExchangeCache>().createView(cache.height())->size();
			});
		});

		manager.addStatelessValidatorHook([](auto& builder) {
			builder
				.add(validators::CreateExchangePluginConfigValidator());
		});

		manager.addStatefulValidatorHook([](auto& builder) {
			builder
				.add(validators::CreateOfferV1Validator())
				.add(validators::CreateOfferV2Validator())
				.add(validators::CreateOfferV3Validator())
				.add(validators::CreateOfferV4Validator())
				.add(validators::CreateExchangeV1Validator())
				.add(validators::CreateExchangeV2Validator())
				.add(validators::CreateRemoveOfferV1Validator())
				.add(validators::CreateRemoveOfferV2Validator());
		});

		manager.addObserverHook([pConfigHolder](auto& builder) {
			builder
				.add(observers::CreateOfferV1Observer())
				.add(observers::CreateOfferV2Observer())
				.add(observers::CreateOfferV3Observer())
				.add(observers::CreateOfferV4Observer())
				.add(observers::CreateExchangeV1Observer())
				.add(observers::CreateExchangeV2Observer())
				.add(observers::CreateRemoveOfferV1Observer(pConfigHolder->Config().Immutable.CurrencyMosaicId))
				.add(observers::CreateRemoveOfferV2Observer(pConfigHolder->Config().Immutable.CurrencyMosaicId))
				.add(observers::CreateAccountV2OfferUpgradeObserver())
				.add(observers::CreateCleanupOffersObserver());
		});
	}
}}

extern "C" PLUGIN_API
void RegisterSubsystem(catapult::plugins::PluginManager& manager) {
	catapult::plugins::RegisterExchangeSubsystem(manager);
}
