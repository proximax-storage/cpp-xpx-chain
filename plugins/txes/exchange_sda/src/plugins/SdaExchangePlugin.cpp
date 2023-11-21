/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/plugins/CacheHandlers.h"
#include "catapult/plugins/PluginManager.h"
#include "SdaExchangePlugin.h"
#include "src/cache/SdaExchangeCache.h"
#include "src/cache/SdaExchangeCacheStorage.h"
#include "src/cache/SdaOfferGroupCache.h"
#include "src/cache/SdaOfferGroupCacheStorage.h"
#include "src/model/SdaExchangeReceiptType.h"
#include "src/observers/Observers.h"
#include "src/plugins/PlaceSdaExchangeOfferTransactionPlugin.h"
#include "src/plugins/RemoveSdaExchangeOfferTransactionPlugin.h"
#include "src/validators/Validators.h"
#include "catapult/observers/ObserverUtils.h"

namespace catapult { namespace plugins {

	void RegisterSdaExchangeSubsystem(PluginManager& manager) {

		manager.addPluginInitializer([](auto& config) {
			config.template InitPluginConfiguration<config::SdaExchangeConfiguration>();
		});

		manager.addTransactionSupport(CreatePlaceSdaExchangeOfferTransactionPlugin());
		manager.addTransactionSupport(CreateRemoveSdaExchangeOfferTransactionPlugin());

		auto pConfigHolder = manager.configHolder();
		manager.addCacheSupport<cache::SdaExchangeCacheStorage>(
			std::make_unique<cache::SdaExchangeCache>(manager.cacheConfig(cache::SdaExchangeCache::Name), pConfigHolder));

		using SdaExchangeCacheHandlersOffer = CacheHandlers<cache::SdaExchangeCacheDescriptor>;
		SdaExchangeCacheHandlersOffer::Register<model::FacilityCode::ExchangeSda>(manager);

		manager.addDiagnosticCounterHook([](auto& counters, const cache::CatapultCache& cache) {
			counters.emplace_back(utils::DiagnosticCounterId("EXCHANGESDA C"), [&cache]() {
				return cache.sub<cache::SdaExchangeCache>().createView(cache.height())->size();
			});
		});

		manager.addCacheSupport<cache::SdaOfferGroupCacheStorage>(
				std::make_unique<cache::SdaOfferGroupCache>(manager.cacheConfig(cache::SdaOfferGroupCache::Name), pConfigHolder));

		using SdaOfferGroupCacheHandlersOffer = CacheHandlers<cache::SdaOfferGroupCacheDescriptor>;
		SdaOfferGroupCacheHandlersOffer::Register<model::FacilityCode::SdaOfferGroup>(manager);

		manager.addDiagnosticCounterHook([](auto& counters, const cache::CatapultCache& cache) {
			counters.emplace_back(utils::DiagnosticCounterId("SDA GR C"), [&cache]() {
				return cache.sub<cache::SdaOfferGroupCache>().createView(cache.height())->size();
			});
		});

		manager.addStatelessValidatorHook([](auto& builder) {
			builder
				.add(validators::CreateSdaExchangePluginConfigValidator());
		});

		manager.addStatefulValidatorHook([](auto& builder) {
			builder
				.add(validators::CreatePlaceSdaExchangeOfferValidator())
				.add(validators::CreateRemoveSdaExchangeOfferValidator());
		});

		manager.addObserverHook([pConfigHolder](auto& builder) {
			builder
				.add(observers::CreatePlaceSdaExchangeOfferObserver())
				.add(observers::CreateRemoveSdaExchangeOfferObserver())
				.add(observers::CreateCleanupSdaOffersObserver());
		});
	}
}}

extern "C" PLUGIN_API
void RegisterSubsystem(catapult::plugins::PluginManager& manager) {
	catapult::plugins::RegisterSdaExchangeSubsystem(manager);
}
