/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#include "MosaicPlugin.h"
#include "MosaicDefinitionTransactionPlugin.h"
#include "MosaicSupplyChangeTransactionPlugin.h"
#include "MosaicModifyLevyTransactionPlugin.h"
#include "MosaicRemoveLevyTransactionPlugin.h"
#include "src/cache/MosaicCache.h"
#include "src/cache/LevyCache.h"
#include "src/cache/MosaicCacheStorage.h"
#include "src/cache/LevyCacheStorage.h"
#include "src/config/MosaicConfiguration.h"
#include "src/model/MosaicReceiptType.h"
#include "src/observers/Observers.h"
#include "src/validators/Validators.h"
#include "catapult/observers/ObserverUtils.h"
#include "catapult/observers/RentalFeeObserver.h"
#include "catapult/plugins/CacheHandlers.h"
#include "catapult/plugins/PluginManager.h"
#include "src/utils/MosaicLevyCalculator.h"
#include "src/catapult/exceptions.h"

namespace catapult { namespace plugins {

	namespace {
		auto GetMosaicView(const cache::CatapultCache& cache) {
			return cache.sub<cache::MosaicCache>().createView(cache.height());
		}
	}

	void RegisterMosaicSubsystem(PluginManager& manager) {
		manager.addPluginInitializer([](auto& config) {
			config.template InitPluginConfiguration<config::MosaicConfiguration>();
		});
		const auto& pConfigHolder = manager.configHolder();
		manager.addTransactionSupport(CreateMosaicDefinitionTransactionPlugin(pConfigHolder));
		manager.addTransactionSupport(CreateMosaicSupplyChangeTransactionPlugin(pConfigHolder));
		manager.addTransactionSupport(CreateMosaicModifyLevyTransactionPlugin());
		manager.addTransactionSupport(CreateMosaicRemoveLevyTransactionPlugin());

		manager.addCacheSupport<cache::MosaicCacheStorage>(
				std::make_unique<cache::MosaicCache>(manager.cacheConfig(cache::MosaicCache::Name)));

		manager.addCacheSupport<cache::LevyCacheStorage>(
			std::make_unique<cache::LevyCache>(manager.cacheConfig(cache::LevyCache::Name), pConfigHolder));

		using CacheHandlersMosaic = CacheHandlers<cache::MosaicCacheDescriptor>;
		CacheHandlersMosaic::Register<model::FacilityCode::Mosaic>(manager);

		manager.addDiagnosticCounterHook([](auto& counters, const cache::CatapultCache& cache) {
			counters.emplace_back(utils::DiagnosticCounterId("MOSAIC C"), [&cache]() { return GetMosaicView(cache)->size(); });
		});

		using CacheHandlersLevy= CacheHandlers<cache::LevyCacheDescriptor>;
		CacheHandlersLevy::Register<model::FacilityCode::Levy>(manager);

		manager.addDiagnosticCounterHook([](auto& counters, const cache::CatapultCache& cache) {
			counters.emplace_back(utils::DiagnosticCounterId("LEVY C"), [&cache]() {
				return cache.sub<cache::LevyCache>().createView(cache.height())->size();
			});
		});

		manager.addStatelessValidatorHook([](auto& builder) {
			builder
				.add(validators::CreateMosaicIdValidator())
				.add(validators::CreateMosaicSupplyChangeValidator())
				.add(validators::CreateMosaicPluginConfigValidator());
		});

		auto currencyMosaicId = config::GetUnresolvedCurrencyMosaicId(manager.immutableConfig());
		manager.addStatefulValidatorHook([currencyMosaicId](auto& builder) {
			builder
				.add(validators::CreateMosaicPropertiesValidator())
				.add(validators::CreateProperMosaicValidator())
				.add(validators::CreateMosaicAvailabilityValidator())
				.add(validators::CreateMosaicDurationValidator())
				.add(validators::CreateMosaicTransferValidator(currencyMosaicId))
				.add(validators::CreateMaxMosaicsBalanceTransferValidator())
				.add(validators::CreateMaxMosaicsSupplyChangeValidator())
				// note that the following validator depends on MosaicChangeAllowedValidator
				.add(validators::CreateMosaicSupplyChangeAllowedValidator())
				.add(validators::CreateModifyLevyValidator())
				.add(validators::CreateRemoveLevyValidator())
				.add(validators::CreateLevyTransferValidator());
		});

		manager.addObserverHook([](auto& builder) {
			auto rentalFeeReceiptType = model::Receipt_Type_Mosaic_Rental_Fee;
			auto expiryReceiptType = model::Receipt_Type_Mosaic_Expired;
			builder
				.add(observers::CreateMosaicDefinitionObserver())
				.add(observers::CreateMosaicSupplyChangeObserver())
				.add(observers::CreateRentalFeeObserver<model::MosaicRentalFeeNotification<1>>("Mosaic", rentalFeeReceiptType))
				.add(observers::CreateCacheBlockTouchObserver<cache::MosaicCache>("Mosaic", expiryReceiptType))
				.add(observers::CreateModifyLevyObserver())
				.add(observers::CreateRemoveLevyObserver())
				.add(observers::CreatePruneLevyHistoryObserver())
				.add(observers::CreateLevyBalanceTransferObserver());
		});
	}
}}

extern "C" PLUGIN_API
void RegisterSubsystem(catapult::plugins::PluginManager& manager) {
	catapult::plugins::RegisterMosaicSubsystem(manager);
}
