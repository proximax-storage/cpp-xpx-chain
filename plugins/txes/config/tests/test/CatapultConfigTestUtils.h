/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "plugins/txes/config/src/cache/CatapultConfigCache.h"
#include "plugins/txes/config/src/cache/CatapultConfigCacheStorage.h"
#include "tests/test/cache/CacheTestUtils.h"

namespace catapult { namespace cache { class CatapultCacheDelta; } }

namespace catapult { namespace test {

	/// Cache factory for creating a catapult cache composed of only the config cache.
	struct CatapultConfigCacheFactory {
		/// Creates an empty catapult cache around \a config.
		static cache::CatapultCache Create(const model::BlockChainConfiguration&) {
			auto cacheId = cache::CatapultConfigCache::Id;
			std::vector<std::unique_ptr<cache::SubCachePlugin>> subCaches(cacheId + 1);
			subCaches[cacheId] = test::MakeSubCachePlugin<cache::CatapultConfigCache, cache::CatapultConfigCacheStorage>();
			return cache::CatapultCache(std::move(subCaches));
		}
	};
}}


