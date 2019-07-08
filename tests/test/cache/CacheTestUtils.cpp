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

#include "CacheTestUtils.h"
#include "catapult/cache/ReadOnlyCatapultCache.h"
#include "catapult/cache/SubCachePluginAdapter.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "catapult/cache_core/AccountStateCacheStorage.h"
#include "catapult/cache_core/BlockDifficultyCacheStorage.h"
#include "catapult/model/BlockChainConfiguration.h"
#include "tests/test/core/mocks/MockLocalNodeConfigurationHolder.h"
#include "tests/test/nodeps/Random.h"

namespace catapult { namespace test {

	namespace {
		Key GetSentinelCachePublicKey() {
			return { { 0xFF, 0xFF, 0xFF, 0xFF } };
		}
	}

	// region CoreSystemCacheFactory

	cache::CatapultCache CoreSystemCacheFactory::Create(const model::BlockChainConfiguration& config) {
		std::vector<std::unique_ptr<cache::SubCachePlugin>> subCaches(2);
		CreateSubCaches(config, subCaches);
		return cache::CatapultCache(std::move(subCaches));
	}

	void CoreSystemCacheFactory::CreateSubCaches(
			const model::BlockChainConfiguration& config,
			std::vector<std::unique_ptr<cache::SubCachePlugin>>& subCaches) {
		CreateSubCaches(config, cache::CacheConfiguration(), subCaches);
	}

	void CoreSystemCacheFactory::CreateSubCaches(
			const model::BlockChainConfiguration& config,
			const cache::CacheConfiguration& cacheConfig,
			std::vector<std::unique_ptr<cache::SubCachePlugin>>& subCaches) {
		using namespace cache;

		auto pConfigHolder = std::make_shared<config::MockLocalNodeConfigurationHolder>();
		const_cast<model::BlockChainConfiguration&>(pConfigHolder->Config(Height{0}).BlockChain) = config;

		subCaches[AccountStateCache::Id] = MakeSubCachePluginWithCacheConfiguration<AccountStateCache, AccountStateCacheStorage>(
				cacheConfig,
				pConfigHolder);

		subCaches[BlockDifficultyCache::Id] = MakeConfigurationFreeSubCachePlugin<BlockDifficultyCache, BlockDifficultyCacheStorage>(pConfigHolder);
	}

	// endregion

	cache::CatapultCache CreateEmptyCatapultCache(const model::BlockChainConfiguration& config) {
		return CreateEmptyCatapultCache<CoreSystemCacheFactory>(config);
	}

	cache::CatapultCache CreateEmptyCatapultCache(
			const model::BlockChainConfiguration& config,
			const cache::CacheConfiguration& cacheConfig) {
		std::vector<std::unique_ptr<cache::SubCachePlugin>> subCaches(2);
		CoreSystemCacheFactory::CreateSubCaches(config, cacheConfig, subCaches);
		return cache::CatapultCache(std::move(subCaches));
	}

	cache::CatapultCache CreateCatapultCacheWithMarkerAccount(const model::BlockChainConfiguration& config) {
		return CreateCatapultCacheWithMarkerAccount(Height(0), config);
	}

	cache::CatapultCache CreateCatapultCacheWithMarkerAccount(Height height, const model::BlockChainConfiguration& config) {
		auto cache = CreateEmptyCatapultCache(config);
		AddMarkerAccount(cache);

		auto delta = cache.createDelta();
		cache.commit(height);
		return cache;
	}

	void AddMarkerAccount(cache::CatapultCache& cache) {
		auto delta = cache.createDelta();
		delta.sub<cache::AccountStateCache>().addAccount(GetSentinelCachePublicKey(), Height(1));
		cache.commit(Height(1));
	}

	namespace {
		template<typename TCache>
		bool IsMarkedCacheT(TCache& cache) {
			const auto& accountStateCache = cache.template sub<cache::AccountStateCache>();
			return 1u == accountStateCache.size() && accountStateCache.contains(GetSentinelCachePublicKey());
		}
	}

	bool IsMarkedCache(const cache::ReadOnlyCatapultCache& cache) {
		return IsMarkedCacheT(cache);
	}

	bool IsMarkedCache(const cache::CatapultCacheDelta& cache) {
		return IsMarkedCacheT(cache);
	}
}}
