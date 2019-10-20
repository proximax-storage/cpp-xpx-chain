/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/plugins/CacheHandlers.h"
#include "catapult/plugins/PluginManager.h"
#include "ExchangePlugin.h"
#include "src/cache/BuyOfferCache.h"
#include "src/cache/BuyOfferCacheStorage.h"
#include "src/cache/DealCache.h"
#include "src/cache/DealCacheStorage.h"
#include "src/cache/SellOfferCache.h"
#include "src/cache/SellOfferCacheStorage.h"
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

		manager.addCacheSupport<cache::BuyOfferCacheStorage>(
			std::make_unique<cache::BuyOfferCache>(manager.cacheConfig(cache::BuyOfferCache::Name)));

		using CacheHandlersBuyOffer = CacheHandlers<cache::BuyOfferCacheDescriptor>;
		CacheHandlersBuyOffer::Register<model::FacilityCode::Exchange>(manager);

		manager.addDiagnosticCounterHook([](auto& counters, const cache::CatapultCache& cache) {
			counters.emplace_back(utils::DiagnosticCounterId("BUYOFFER C"), [&cache]() {
				return cache.sub<cache::BuyOfferCache>().createView(cache.height())->size();
			});
		});

		manager.addCacheSupport<cache::SellOfferCacheStorage>(
			std::make_unique<cache::SellOfferCache>(manager.cacheConfig(cache::SellOfferCache::Name)));

		using CacheHandlersSellOffer = CacheHandlers<cache::SellOfferCacheDescriptor>;
		CacheHandlersSellOffer::Register<model::FacilityCode::Exchange>(manager);

		manager.addDiagnosticCounterHook([](auto& counters, const cache::CatapultCache& cache) {
			counters.emplace_back(utils::DiagnosticCounterId("SELLOFFER C"), [&cache]() {
				return cache.sub<cache::SellOfferCache>().createView(cache.height())->size();
			});
		});

		manager.addCacheSupport<cache::DealCacheStorage>(
			std::make_unique<cache::DealCache>(manager.cacheConfig(cache::DealCache::Name)));

		using CacheHandlersDeal = CacheHandlers<cache::DealCacheDescriptor>;
		CacheHandlersDeal::Register<model::FacilityCode::Exchange>(manager);

		manager.addDiagnosticCounterHook([](auto& counters, const cache::CatapultCache& cache) {
			counters.emplace_back(utils::DiagnosticCounterId("DEAL C"), [&cache]() {
				return cache.sub<cache::DealCache>().createView(cache.height())->size();
			});
		});

		manager.addStatefulValidatorHook([config = manager.immutableConfig()](auto& builder) {
			builder
				.add(validators::CreateBuyOfferValidator(config))
				.add(validators::CreateSellOfferValidator(config))
				.add(validators::CreateMatchedBuyOfferValidator())
				.add(validators::CreateMatchedSellOfferValidator())
				.add(validators::CreateRemoveOfferValidator());
		});

		manager.addObserverHook([&manager](auto& builder) {
			builder
				.add(observers::CreateBuyOfferObserver())
				.add(observers::CreateMatchedBuyOfferObserver())
				.add(observers::CreateSellOfferObserver())
				.add(observers::CreateMatchedSellOfferObserver())
				.add(observers::CreateRemoveOfferObserver())
				.add(observers::CreateCleanupOffersObserver(manager));
		});
	}
}}

extern "C" PLUGIN_API
void RegisterSubsystem(catapult::plugins::PluginManager& manager) {
	catapult::plugins::RegisterExchangeSubsystem(manager);
}
