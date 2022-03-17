/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/cache_core/AccountStateCache.h"
#include "src/cache/LockFundCache.h"
#include "src/cache/LockFundCacheStorage.h"
#include "catapult/cache_core/AccountStateCacheStorage.h"
#include "catapult/cache_core/BlockDifficultyCacheStorage.h"
#include "catapult/cache/SubCachePlugin.h"
#include "LockFundCacheFactory.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "catapult/cache/CatapultCache.h"
#include "tests/test/cache/CacheTestUtils.h"
#include "tests/test/other/MutableBlockchainConfiguration.h"
namespace catapult { namespace test {

	namespace {
		cache::AccountStateCacheTypes::Options CreateAccountStateCacheOptions(const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder) {
			const auto& config = pConfigHolder->Config().Immutable;
			return {
					pConfigHolder,
					config.NetworkIdentifier,
					config.CurrencyMosaicId,
					config.HarvestingMosaicId
			};
		}
	}

	cache::CatapultCache LockFundCacheFactory::Create() {
		return Create(test::MutableBlockchainConfiguration().ToConst());
	}

	cache::CatapultCache LockFundCacheFactory::Create(const config::BlockchainConfiguration& config) {
		std::vector<std::unique_ptr<cache::SubCachePlugin>> subCaches(20);
		CreateSubCaches(config, cache::CacheConfiguration(), subCaches);
		return cache::CatapultCache(std::move(subCaches));
	}
	void LockFundCacheFactory::CreateSubCaches(
			const config::BlockchainConfiguration& config,
			std::vector<std::unique_ptr<cache::SubCachePlugin>>& subCaches) {
		CreateSubCaches(config, cache::CacheConfiguration(), subCaches);
	}
	void LockFundCacheFactory::CreateSubCaches(
			const config::BlockchainConfiguration& config,
			const cache::CacheConfiguration& cacheConfig,
			std::vector<std::unique_ptr<cache::SubCachePlugin>>& subCaches) {
		using namespace cache;

		auto pConfigHolder = config::CreateMockConfigurationHolder(config);
		subCaches[AccountStateCache::Id] = MakeSubCachePluginWithCacheConfiguration<cache::AccountStateCache, AccountStateCacheStorage>(
				cacheConfig,
				CreateAccountStateCacheOptions(pConfigHolder));

		subCaches[BlockDifficultyCache::Id] = MakeConfigurationFreeSubCachePlugin<BlockDifficultyCache, BlockDifficultyCacheStorage>(pConfigHolder);

		subCaches[LockFundCache::Id] = MakeSubCachePluginWithCacheConfiguration<LockFundCache, LockFundCacheStorage>(cacheConfig, pConfigHolder);
	}

}}