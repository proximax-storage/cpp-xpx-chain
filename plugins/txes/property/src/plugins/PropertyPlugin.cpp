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

#include "PropertyPlugin.h"
#include "src/cache/PropertyCache.h"
#include "src/cache/PropertyCacheStorage.h"
#include "src/config/PropertyConfiguration.h"
#include "src/observers/Observers.h"
#include "src/plugins/PropertyTransactionPlugin.h"
#include "src/validators/Validators.h"
#include "catapult/plugins/CacheHandlers.h"

namespace catapult { namespace plugins {

	void RegisterPropertySubsystem(PluginManager& manager) {
		manager.addPluginInitializer([](auto& config) {
			config.template InitPluginConfiguration<config::PropertyConfiguration>();
		});
		manager.addTransactionSupport(CreateAddressPropertyTransactionPlugin());
		manager.addTransactionSupport(CreateMosaicPropertyTransactionPlugin());
		manager.addTransactionSupport(CreateTransactionTypePropertyTransactionPlugin());

		const auto& pConfigHolder = manager.configHolder();
		manager.addCacheSupport<cache::PropertyCacheStorage>(
				std::make_unique<cache::PropertyCache>(manager.cacheConfig(cache::PropertyCache::Name), pConfigHolder));

		using CacheHandlers = CacheHandlers<cache::PropertyCacheDescriptor>;
		CacheHandlers::Register<model::FacilityCode::Property>(manager);

		manager.addDiagnosticCounterHook([](auto& counters, const cache::CatapultCache& cache) {
			counters.emplace_back(utils::DiagnosticCounterId("PROPERTY C"), [&cache]() {
				return cache.sub<cache::PropertyCache>().createView(cache.height())->size();
			});
		});

		auto networkIdentifier = pConfigHolder->Config().Immutable.NetworkIdentifier;
		manager.addStatelessValidatorHook([networkIdentifier](auto& builder) {
			builder
				.add(validators::CreatePropertyAddressNoSelfModificationValidator(networkIdentifier))
				.add(validators::CreatePropertyTypeValidator())
				.add(validators::CreateAddressPropertyModificationTypesValidator())
				.add(validators::CreateMosaicPropertyModificationTypesValidator())
				.add(validators::CreateTransactionTypePropertyModificationTypesValidator())
				.add(validators::CreateTransactionTypePropertyModificationValuesValidator())
				.add(validators::CreatePropertyPluginConfigValidator());
		});

		manager.addStatefulValidatorHook([](auto& builder) {
			builder
				.add(validators::CreateAddressPropertyRedundantModificationValidator())
				.add(validators::CreateAddressPropertyValueModificationValidator())
				.add(validators::CreateMaxAddressPropertyValuesValidator())
				.add(validators::CreateAddressInteractionValidator())
				.add(validators::CreateMosaicPropertyRedundantModificationValidator())
				.add(validators::CreateMosaicPropertyValueModificationValidator())
				.add(validators::CreateMaxMosaicPropertyValuesValidator())
				.add(validators::CreateMosaicRecipientValidator())
				.add(validators::CreateTransactionTypePropertyRedundantModificationValidator())
				.add(validators::CreateTransactionTypePropertyValueModificationValidator())
				.add(validators::CreateMaxTransactionTypePropertyValuesValidator())
				.add(validators::CreateTransactionTypeValidator())
				.add(validators::CreateTransactionTypeNoSelfBlockingValidator());
		});

		manager.addObserverHook([](auto& builder) {
			builder
				.add(observers::CreateAddressPropertyValueModificationObserver())
				.add(observers::CreateMosaicPropertyValueModificationObserver())
				.add(observers::CreateTransactionTypePropertyValueModificationObserver());
		});
	}
}}

extern "C" PLUGIN_API
void RegisterSubsystem(catapult::plugins::PluginManager& manager) {
	catapult::plugins::RegisterPropertySubsystem(manager);
}
