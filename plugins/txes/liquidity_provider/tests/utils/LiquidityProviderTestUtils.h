/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once

#include <tests/test/core/mocks/MockBlockchainConfigurationHolder.h>
#include <src/cache/LiquidityProviderCacheStorage.h>
#include <catapult/cache_core/AccountStateCacheDelta.h>
#include <tests/test/other/MutableBlockchainConfiguration.h>
#include <tests/test/cache/CacheTestUtils.h>
#include "src/cache/LiquidityProviderKeyCollector.h"
#include "src/cache/LiquidityProviderCache.h"
namespace catapult::test {

	/// Cache factory for creating a catapult cache composed of bc drive cache and core caches.
	struct LiquidityProviderCacheFactory {
	private:
		static auto CreateSubCacheWithLiquidityProviderCache(const config::BlockchainConfiguration& config) {
			auto id = cache::LiquidityProviderCache::Id;
			std::vector<std::unique_ptr<cache::SubCachePlugin>> subCaches(id + 1);
			auto pConfigHolder = config::CreateMockConfigurationHolder(config);
			auto pKeyCollector = std::make_shared<cache::LiquidityProviderKeyCollector>();
			subCaches[cache::LiquidityProviderCache::Id] = MakeSubCachePlugin<cache::LiquidityProviderCache, cache::LiquidityProviderCacheStorage>(
					pKeyCollector, pConfigHolder);
			return subCaches;
		}

	public:
		/// Creates an empty catapult cache around default configuration.
		static cache::CatapultCache Create() {
			return Create(test::MutableBlockchainConfiguration().ToConst());
		}

		/// Creates an empty catapult cache around \a config.
		static cache::CatapultCache Create(const config::BlockchainConfiguration& config) {
			auto subCaches = CreateSubCacheWithLiquidityProviderCache(config);
			CoreSystemCacheFactory::CreateSubCaches(config, subCaches);
			return cache::CatapultCache(std::move(subCaches));
		}
	};

	/// Adds account state with \a publicKey and provided \a mosaics to \a accountStateCache at height \a height.
	void AddAccountState(
			cache::AccountStateCacheDelta& accountStateCache,
			const Key& publicKey,
			const Height& height = Height(1),
			const std::vector<model::Mosaic>& mosaics = {});
} // namespace catapult::test