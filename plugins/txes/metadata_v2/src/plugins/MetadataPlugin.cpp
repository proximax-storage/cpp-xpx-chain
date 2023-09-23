/**
*** Copyright (c) 2016-2019, Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp.
*** Copyright (c) 2020-present, Jaguar0625, gimre, BloodyRookie.
*** All rights reserved.
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

#include "MetadataPlugin.h"
#include "MetadataTransactionPlugin.h"
#include "src/cache/MetadataCache.h"
#include "src/cache/MetadataCacheStorage.h"
#include "src/observers/Observers.h"
#include "src/validators/Validators.h"
#include "catapult/plugins/CacheHandlers.h"

namespace catapult { namespace plugins {

	void RegisterMetadataV2Subsystem(PluginManager& manager) {
		manager.addPluginInitializer([](auto& config) {
			config.template InitPluginConfiguration<config::MetadataConfiguration>();
		});

		manager.addTransactionSupport(CreateAccountMetadataTransactionPlugin(manager.configHolder()));
		manager.addTransactionSupport(CreateMosaicMetadataTransactionPlugin(manager.configHolder()));
		manager.addTransactionSupport(CreateNamespaceMetadataTransactionPlugin(manager.configHolder()));

        const auto& pConfigHolder = manager.configHolder();
		manager.addCacheSupport<cache::MetadataCacheStorage>(std::make_unique<cache::MetadataCache>(
				manager.cacheConfig(cache::MetadataCache::Name), pConfigHolder));

		using CacheHandlers = CacheHandlers<cache::MetadataCacheDescriptor>;
		CacheHandlers::Register<model::FacilityCode::Metadata_v2>(manager);

		manager.addDiagnosticCounterHook([](auto& counters, const cache::CatapultCache& cache) {
			counters.emplace_back(utils::DiagnosticCounterId("METADATAN C"), [&cache]() {
				return cache.sub<cache::MetadataCache>().createView(cache.height())->size();
			});
		});

		manager.addStatefulValidatorHook([](auto& builder) {
			builder
				.add(validators::CreateMetadataValueValidator())
				.add(validators::CreateMetadataSizesValidator());
		});

		manager.addObserverHook([](auto& builder) {
			builder.add(observers::CreateMetadataValueObserver());
		});
	}
}}

extern "C" PLUGIN_API
void RegisterSubsystem(catapult::plugins::PluginManager& manager) {
	catapult::plugins::RegisterMetadataV2Subsystem(manager);
}
