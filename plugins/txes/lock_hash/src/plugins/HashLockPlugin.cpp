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

#include "HashLockPlugin.h"
#include "src/cache/HashLockInfoCache.h"
#include "src/model/HashLockReceiptType.h"
#include "src/observers/Observers.h"
#include "src/plugins/HashLockTransactionPlugin.h"
#include "src/validators/Validators.h"
#include "catapult/observers/ObserverUtils.h"
#include "catapult/plugins/CacheHandlers.h"

namespace catapult { namespace plugins {

	void RegisterHashLockSubsystem(PluginManager& manager) {
		manager.addPluginInitializer([](auto& config) {
			config.template InitPluginConfiguration<config::HashLockConfiguration>();
		});
		manager.addTransactionSupport(CreateHashLockTransactionPlugin());

		manager.addCacheSupport<cache::HashLockInfoCacheStorage>(
				std::make_unique<cache::HashLockInfoCache>(manager.cacheConfig(cache::HashLockInfoCache::Name)));

		using CacheHandlers = CacheHandlers<cache::HashLockInfoCacheDescriptor>;
		CacheHandlers::Register<model::FacilityCode::LockHash>(manager);

		manager.addDiagnosticCounterHook([](auto& counters, const cache::CatapultCache& cache) {
			counters.emplace_back(utils::DiagnosticCounterId("HASHLOCK C"), [&cache]() {
				return cache.sub<cache::HashLockInfoCache>().createView(cache.height())->size();
			});
		});

		manager.addStatelessValidatorHook([](auto& builder) {
			builder
				.add(validators::CreateHashLockPluginConfigValidator());
		});

		manager.addStatefulValidatorHook([](auto& builder) {
			builder
				.add(validators::CreateHashLockDurationValidator())
				.add(validators::CreateHashLockMosaicValidator())
				.add(validators::CreateAggregateHashPresentValidator())
				.add(validators::CreateHashLockCacheUniqueValidator());
		});

		manager.addObserverHook([](auto& builder) {
			auto expiryReceiptType = model::Receipt_Type_LockHash_Expired;
			builder
				.add(observers::CreateHashLockObserver())
				.add(observers::CreateExpiredHashLockInfoObserver())
				.add(observers::CreateCacheBlockTouchObserver<cache::HashLockInfoCache>("HashLockInfo", expiryReceiptType))
				.add(observers::CreateCacheBlockPruningObserver<cache::HashLockInfoCache>("HashLockInfo", 1))
				.add(observers::CreateCompletedAggregateObserver());
		});
	}
}}

extern "C" PLUGIN_API
void RegisterSubsystem(catapult::plugins::PluginManager& manager) {
	catapult::plugins::RegisterHashLockSubsystem(manager);
}
