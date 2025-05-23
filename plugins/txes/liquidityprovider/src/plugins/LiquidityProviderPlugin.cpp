/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/cache/LiquidityProviderCacheSubCachePlugin.h"
#include "src/validators/LiquidityProviderExchangeValidatorImpl.h"
#include "src/observers/LiquidityProviderExchangeObserverImpl.h"
#include "CreateLiquidityProviderTransactionPlugin.h"
#include "ManualRateChangeTransactionPlugin.h"
#include "src/validators/Validators.h"
#include "src/observers/Observers.h"
#include "src/config/LiquidityProviderConfiguration.h"
#include "catapult/plugins/CacheHandlers.h"
#include "LiquidityProviderPlugin.h"

namespace catapult { namespace plugins {

	void RegisterLiquidityProviderSubsystem(PluginManager& manager) {
		manager.addPluginInitializer([](auto& config) {
			config.template InitPluginConfiguration<config::LiquidityProviderConfiguration>();
		});

		const auto& pConfigHolder = manager.configHolder();
		const auto& immutableConfig = manager.immutableConfig();

		manager.addTransactionSupport(CreateCreateLiquidityProviderTransactionPlugin(immutableConfig));
		manager.addTransactionSupport(CreateManualRateChangeTransactionPlugin());

		manager.setLiquidityProviderExchangeValidator(std::make_unique<validators::LiquidityProviderExchangeValidatorImpl>());
		manager.setLiquidityProviderExchangeObserver(std::make_unique<observers::LiquidityProviderExchangeObserverImpl>());

		auto pKeyCollector = std::make_shared<cache::LiquidityProviderKeyCollector>();
		manager.addCacheSupport(std::make_unique<cache::LiquidityProviderCacheSubCachePlugin>(
			manager.cacheConfig(cache::LiquidityProviderCache::Name), pKeyCollector, pConfigHolder));

		using LiquidityProviderCacheHandlersService = CacheHandlers<cache::LiquidityProviderCacheDescriptor>;
		LiquidityProviderCacheHandlersService::Register<model::FacilityCode::LiquidityProvider>(manager);

		manager.addDiagnosticCounterHook([](auto& counters, const cache::CatapultCache& cache) {
			counters.emplace_back(utils::DiagnosticCounterId("LP C"), [&cache]() {
				return cache.sub<cache::LiquidityProviderCache>().createView(cache.height())->size();
			});
		});

		manager.addStatelessValidatorHook([](auto& builder) {
			builder
			.add(validators::CreateLiquidityProviderPluginConfigValidator());
		});

		const auto& liquidityProviderValidator = manager.liquidityProviderExchangeValidator();
		manager.addStatefulValidatorHook([pConfigHolder, &immutableConfig, &liquidityProviderValidator](auto& builder) {
			builder
			.add(validators::CreateCreateLiquidityProviderValidator())
			.add(validators::CreateCreditMosaicValidator(liquidityProviderValidator))
			.add(validators::CreateDebitMosaicValidator(liquidityProviderValidator))
			.add(validators::CreateManualRateChangeValidator());
		});

		const auto& liquidityProviderObserver = manager.liquidityProviderExchangeObserver();
		manager.addObserverHook([pKeyCollector, &liquidityProviderObserver] (auto& builder) {
			builder
			.add(observers::CreateSlashingObserver(pKeyCollector))
			.add(observers::CreateCreateLiquidityProviderObserver())
			.add(observers::CreateCreditMosaicObserver(liquidityProviderObserver))
			.add(observers::CreateDebitMosaicObserver(liquidityProviderObserver))
			.add(observers::CreateManualRateChangeObserver());
		});
	}
}}


extern "C" PLUGIN_API
void RegisterSubsystem(catapult::plugins::PluginManager& manager) {
	catapult::plugins::RegisterLiquidityProviderSubsystem(manager);
}
