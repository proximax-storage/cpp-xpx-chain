/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "plugins/txes/config/src/cache/NetworkConfigCache.h"
#include "plugins/txes/config/src/cache/NetworkConfigCacheStorage.h"
#include "tests/test/cache/CacheTestUtils.h"

namespace catapult { namespace cache { class CatapultCacheDelta; } }

namespace catapult { namespace test {

	/// Cache factory for creating a catapult cache composed of only the config cache.
	struct NetworkConfigCacheFactory {
		/// Creates an empty catapult cache.
		static cache::CatapultCache Create() {
			auto cacheId = cache::NetworkConfigCache::Id;
			std::vector<std::unique_ptr<cache::SubCachePlugin>> subCaches(cacheId + 1);
			subCaches[cacheId] = test::MakeSubCachePlugin<cache::NetworkConfigCache, cache::NetworkConfigCacheStorage>();
			return cache::CatapultCache(std::move(subCaches));
		}
	};
}}


