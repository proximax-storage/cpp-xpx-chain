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

#include "SecretLockPlugin.h"
#include "src/cache/SecretLockInfoCache.h"
#include "src/config/SecretLockConfiguration.h"
#include "src/model/SecretLockReceiptType.h"
#include "src/observers/Observers.h"
#include "src/plugins/SecretLockTransactionPlugin.h"
#include "src/plugins/SecretProofTransactionPlugin.h"
#include "src/validators/Validators.h"
#include "catapult/observers/ObserverUtils.h"
#include "catapult/plugins/CacheHandlers.h"

namespace catapult { namespace plugins {

	void RegisterSecretLockSubsystem(PluginManager& manager) {
		manager.addPluginInitializer([](auto& config) {
			config.template InitPluginConfiguration<config::SecretLockConfiguration>();
		});
		manager.addTransactionSupport(CreateSecretProofTransactionPlugin());
		manager.addTransactionSupport(CreateSecretLockTransactionPlugin());

		manager.addCacheSupport<cache::SecretLockInfoCacheStorage>(
				std::make_unique<cache::SecretLockInfoCache>(manager.cacheConfig(cache::SecretLockInfoCache::Name)));

		using CacheHandlers = CacheHandlers<cache::SecretLockInfoCacheDescriptor>;
		CacheHandlers::Register<model::FacilityCode::LockSecret>(manager);

		manager.addDiagnosticCounterHook([](auto& counters, const cache::CatapultCache& cache) {
			counters.emplace_back(utils::DiagnosticCounterId("SECRETLOCK C"), [&cache]() {
				return cache.sub<cache::SecretLockInfoCache>().createView(cache.height())->size();
			});
		});

		manager.addStatelessValidatorHook([](auto& builder) {
			builder
				.add(validators::CreateSecretLockHashAlgorithmValidator())
				.add(validators::CreateSecretLockPluginConfigValidator());
		});

		manager.addStatefulValidatorHook([](auto& builder) {
			builder
				.add(validators::CreateSecretLockDurationValidator())
				.add(validators::CreateProofSecretValidator())
				.add(validators::CreateSecretLockCacheUniqueValidator())
				.add(validators::CreateProofValidator());
		});

		manager.addObserverHook([](auto& builder) {
			auto expiryReceiptType = model::Receipt_Type_LockSecret_Expired;
			builder
				.add(observers::CreateSecretLockObserver())
				.add(observers::CreateExpiredSecretLockInfoObserver())
				.add(observers::CreateProofObserver())
				.add(observers::CreateCacheBlockTouchObserver<cache::SecretLockInfoCache>("SecretLockInfo", expiryReceiptType))
				.add(observers::CreateCacheBlockPruningObserver<cache::SecretLockInfoCache>("SecretLockInfo", 1));
		});
	}
}}

extern "C" PLUGIN_API
void RegisterSubsystem(catapult::plugins::PluginManager& manager) {
	catapult::plugins::RegisterSecretLockSubsystem(manager);
}
