/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/cache/MetadataCache.h"
#include "src/cache/MetadataCacheStorage.h"
#include "plugins/txes/mosaic/src/cache/MosaicCache.h"
#include "plugins/txes/namespace/src/cache/NamespaceCache.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "catapult/cache_core/AccountStateCacheStorage.h"
#include "catapult/model/Address.h"
#include "catapult/model/NetworkConfiguration.h"
#include "catapult/plugins/PluginUtils.h"
#include "tests/test/cache/CacheTestUtils.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "tests/test/other/MutableBlockchainConfiguration.h"
#include "tests/TestHarness.h"

namespace catapult { namespace test {

	/// Cache factory for creating a catapult cache composed of account cache and core caches.
	struct MetadataCacheFactory {
	private:
		static auto CreateSubCachesWithMetadataCache(const config::BlockchainConfiguration& config) {
			std::vector<std::unique_ptr<cache::SubCachePlugin>> subCaches(cache::MosaicCache::Id + 1);
			auto pConfigHolder = config::CreateMockConfigurationHolder(config);

			subCaches[cache::MetadataCache::Id] =
					MakeSubCachePlugin<cache::MetadataCache, cache::MetadataCacheStorage>();
			subCaches[cache::MosaicCache::Id] =
					MakeSubCachePlugin<cache::MosaicCache, cache::MosaicCacheStorage>();
			subCaches[cache::NamespaceCache::Id] =
					MakeSubCachePlugin<cache::NamespaceCache, cache::NamespaceCacheStorage>(cache::NamespaceCacheTypes::Options{ pConfigHolder });
			return subCaches;
		}

	public:
		/// Creates an empty catapult cache.
		static cache::CatapultCache Create() {
			return Create(test::MutableBlockchainConfiguration().ToConst());
		}

		/// Creates an empty catapult cache around \a config.
		static cache::CatapultCache Create(const config::BlockchainConfiguration& config) {
			auto subCaches = CreateSubCachesWithMetadataCache(config);
			CoreSystemCacheFactory::CreateSubCaches(config, subCaches);
			return cache::CatapultCache(std::move(subCaches));
		}
	};

	/// Asserts that \a expected and \a actual are equal.
	inline void AssertEqual(const state::MetadataEntry& expected, const state::MetadataEntry& actual) {
		EXPECT_EQ(expected.metadataId(), actual.metadataId());
		EXPECT_EQ(expected.raw(), actual.raw());
		EXPECT_EQ(expected.type(), actual.type());
		EXPECT_EQ(expected.fields(), actual.fields());
	}
}}
