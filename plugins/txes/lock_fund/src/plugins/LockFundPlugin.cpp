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
