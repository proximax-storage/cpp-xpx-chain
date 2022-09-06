/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "GlobalStoreCacheSystem.h"
#include "src/cache/GlobalStoreCacheStorage.h"
#include "src/cache/GlobalStoreCache.h"
#include "catapult/plugins/PluginManager.h"

namespace catapult { namespace plugins {

	void RegisterGlobalStoreCacheSystem(PluginManager& manager) {
		const auto& pConfigHolder = manager.configHolder();
		manager.addCacheSupport<cache::GlobalStoreCacheStorage>(std::make_unique<cache::GlobalStoreCache>(
				manager.cacheConfig(cache::GlobalStoreCache::Name), pConfigHolder));
	}
}}

extern "C" PLUGIN_API
void RegisterSubsystem(catapult::plugins::PluginManager& manager) {
	catapult::plugins::RegisterGlobalStoreCacheSystem(manager);
}
