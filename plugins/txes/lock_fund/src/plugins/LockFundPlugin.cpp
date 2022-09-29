/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/cache/LockFundCacheStorage.h"
#include "LockFundPlugin.h"
#include "src/cache/LockFundCache.h"
#include "LockFundTransferTransactionPlugin.h"
#include "LockFundCancelUnlockTransactionPlugin.h"
#include "src/config/LockFundConfiguration.h"
#include "src/validators/Validators.h"
#include "catapult/plugins/PluginManager.h"
#include "src/observers/Observers.h"


namespace catapult { namespace plugins {

	void RegisterLockFundSubsystem(PluginManager& manager) {
		manager.addPluginInitializer([](auto& config) {
			config.template InitPluginConfiguration<config::LockFundConfiguration>();
		});
		const auto& pConfigHolder = manager.configHolder();
		manager.addTransactionSupport(CreateLockFundTransferTransactionPlugin());
		manager.addTransactionSupport(CreateLockFundCancelUnlockTransactionPlugin());
		manager.addStatelessValidatorHook([](auto& builder) {
			builder
				.add(validators::CreateLockFundPluginConfigValidator());
		});
		manager.addCacheSupport<cache::LockFundCacheStorage>(
				std::make_unique<cache::LockFundCache>(manager.cacheConfig(cache::LockFundCache::Name), pConfigHolder));
		manager.addStatefulValidatorHook([](auto& builder) {
			builder
				.add(validators::CreateLockFundTransferValidator())
				.add(validators::CreateLockFundCancelUnlockValidator());
		});
		manager.addObserverHook([](auto& builder) {
		  builder.add(observers::CreateLockFundTransferObserver())
				.add(observers::CreateLockFundBlockObserver())
				.add(observers::CreateLockFundCancelUnlockObserver());
		});
	}
}}

extern "C" PLUGIN_API
void RegisterSubsystem(catapult::plugins::PluginManager& manager) {
	catapult::plugins::RegisterLockFundSubsystem(manager);
}
