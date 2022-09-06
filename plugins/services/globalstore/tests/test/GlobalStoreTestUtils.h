/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/


#pragma once
#include "tests/test/other/MutableBlockchainConfiguration.h"
#include "src/cache/GlobalStoreCacheStorage.h"
#include "src/cache/GlobalStoreCache.h";
#include "catapult/model/NetworkConfiguration.h"
#include "tests/test/cache/CacheTestUtils.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"

namespace catapult { namespace test {

	/// Cache factory for creating a catapult cache composed of only the global store.
	struct HashCacheFactory {
		/// Creates an empty catapult cache.
		static cache::CatapultCache Create() {
			auto cacheId = cache::GlobalStoreCache::Id;
			std::vector<std::unique_ptr<cache::SubCachePlugin>> subCaches(cacheId + 1);
			auto pConfigHolder = config::CreateMockConfigurationHolder();
			subCaches[cacheId] = MakeSubCachePlugin<cache::GlobalStoreCache, cache::GlobalStoreCacheStorage>(pConfigHolder);
			return cache::CatapultCache(std::move(subCaches));
		}


	};
	std::shared_ptr<config::BlockchainConfigurationHolder> CreateGlobalStoreConfigHolder(model::NetworkIdentifier networkIdentifier = model::NetworkIdentifier::Zero);

	state::GlobalEntry CreateGlobalEntry(const Hash256& key);

	void AssertEqual(const state::GlobalEntry& expected, const state::GlobalEntry& actual);
}}
