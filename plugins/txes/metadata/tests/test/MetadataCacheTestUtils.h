/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/cache/MetadataCache.h"
#include "src/cache/MetadataCacheStorage.h"
#include "src/config/NamespaceConfiguration.h"
#include "plugins/txes/mosaic/src/cache/MosaicCache.h"
#include "plugins/txes/namespace/src/cache/NamespaceCache.h"
#include "catapult/model/Address.h"
#include "catapult/model/BlockChainConfiguration.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "catapult/cache_core/AccountStateCacheStorage.h"
#include "tests/test/cache/CacheTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace test {

	/// Cache factory for creating a catapult cache composed of account cache and core caches.
	struct MetadataCacheFactory {
	private:
		static auto CreateSubCachesWithMetadataCache(const model::BlockChainConfiguration& blockChainConfig) {
			std::vector<std::unique_ptr<cache::SubCachePlugin>> subCaches(cache::MosaicCache::Id + 1);
			auto pluginConfig = config::NamespaceConfiguration::Uninitialized();
			const_cast<model::BlockChainConfiguration&>(blockChainConfig).SetPluginConfiguration("catapult.plugins.metadata", pluginConfig);
			auto pConfigHolder = std::make_shared<config::LocalNodeConfigurationHolder>();
			pConfigHolder->SetBlockChainConfig(Height{0}, blockChainConfig);

			subCaches[cache::MetadataCache::Id] =
					MakeSubCachePlugin<cache::MetadataCache, cache::MetadataCacheStorage>();
			subCaches[cache::MosaicCache::Id] =
					MakeSubCachePlugin<cache::MosaicCache, cache::MosaicCacheStorage>();
			subCaches[cache::NamespaceCache::Id] =
					MakeSubCachePlugin<cache::NamespaceCache, cache::NamespaceCacheStorage>(cache::NamespaceCacheTypes::Options{ pConfigHolder });
			return subCaches;
		}

	public:
		/// Creates an empty catapult cache around \a config.
		static cache::CatapultCache Create(const model::BlockChainConfiguration& config) {
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
