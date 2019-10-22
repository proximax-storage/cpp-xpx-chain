/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/plugins/CacheHandlers.h"
#include "catapult/plugins/PluginManager.h"
#include "ExchangePlugin.h"
#include "src/cache/OfferCache.h"
#include "src/cache/OfferCacheStorage.h"
#include "src/observers/Observers.h"
#include "src/plugins/BuyOfferTransactionPlugin.h"
#include "src/plugins/RemoveOfferTransactionPlugin.h"
#include "src/plugins/SellOfferTransactionPlugin.h"
#include "src/validators/Validators.h"

namespace catapult { namespace plugins {

	void RegisterExchangeSubsystem(PluginManager& manager) {
		manager.addTransactionSupport(CreateBuyOfferTransactionPlugin());
		manager.addTransactionSupport(CreateSellOfferTransactionPlugin());
		manager.addTransactionSupport(CreateRemoveOfferTransactionPlugin());

		auto pConfigHolder = manager.configHolder();
		manager.addCacheSupport<cache::OfferCacheStorage>(
			std::make_unique<cache::OfferCache>(manager.cacheConfig(cache::OfferCache::Name), pConfigHolder));

		using CacheHandlersOffer = CacheHandlers<cache::OfferCacheDescriptor>;
		CacheHandlersOffer::Register<model::FacilityCode::Exchange>(manager);

		manager.addDiagnosticCounterHook([](auto& counters, const cache::CatapultCache& cache) {
			counters.emplace_back(utils::DiagnosticCounterId("OFFER C"), [&cache]() {
				return cache.sub<cache::OfferCache>().createView(cache.height())->size();
			});
		});

		manager.addStatelessValidatorHook([](auto& builder) {
			builder
				.add(validators::CreateExchangePluginConfigValidator());
		});

		manager.addStatefulValidatorHook([pConfigHolder](auto& builder) {
			builder
				.add(validators::CreateOfferValidator(pConfigHolder))
				.add(validators::CreateMatchedOfferValidator())
				.add(validators::CreateRemoveOfferValidator());
		});

		manager.addObserverHook([pConfigHolder](auto& builder) {
			builder
				.add(observers::CreateOfferObserver())
				.add(observers::CreateMatchedOfferObserver())
				.add(observers::CreateRemoveOfferObserver())
				.add(observers::CreateCleanupOffersObserver(pConfigHolder));
		});
	}
}}

extern "C" PLUGIN_API
void RegisterSubsystem(catapult::plugins::PluginManager& manager) {
	catapult::plugins::RegisterExchangeSubsystem(manager);
}
