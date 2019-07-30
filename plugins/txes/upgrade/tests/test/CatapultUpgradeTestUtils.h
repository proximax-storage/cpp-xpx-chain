/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/cache/CatapultUpgradeCache.h"
#include "src/cache/CatapultUpgradeCacheStorage.h"
#include "tests/test/cache/CacheTestUtils.h"

namespace catapult { namespace cache { class CatapultCacheDelta; } }

namespace catapult { namespace test {

	/// Cache factory for creating a catapult cache composed of only the config cache.
	struct CatapultUpgradeCacheFactory {
		/// Creates an empty catapult cache around \a config.
		static cache::CatapultCache Create(const model::BlockChainConfiguration&) {
			auto cacheId = cache::CatapultUpgradeCache::Id;
			std::vector<std::unique_ptr<cache::SubCachePlugin>> subCaches(cacheId + 1);
			subCaches[cacheId] = test::MakeSubCachePlugin<cache::CatapultUpgradeCache, cache::CatapultUpgradeCacheStorage>();
			return cache::CatapultCache(std::move(subCaches));
		}
	};
}}


