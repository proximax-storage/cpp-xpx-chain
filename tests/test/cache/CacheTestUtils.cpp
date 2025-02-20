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
#include "catapult/cache_core/AccountStateCache.h"
#include "catapult/cache_core/AccountStateCacheStorage.h"
#include "catapult/cache_core/BlockDifficultyCacheStorage.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "tests/test/other/MutableBlockchainConfiguration.h"
#include "plugins/txes/config/src/cache/NetworkConfigCache.h"
#include "plugins/txes/config/src/cache/NetworkConfigCacheStorage.h"

namespace catapult { namespace test {

	namespace {
		Key GetSentinelCachePublicKey() {
			return { { 0xFF, 0xFF, 0xFF, 0xFF } };
		}

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

	// region CoreSystemCacheFactory

	cache::CatapultCache CoreSystemCacheFactory::Create() {
		return Create(test::MutableBlockchainConfiguration().ToConst());
	}

	cache::CatapultCache CoreSystemCacheFactory::Create(const config::BlockchainConfiguration& config) {
		std::vector<std::unique_ptr<cache::SubCachePlugin>> subCaches(3);
		CreateSubCaches(config, subCaches);
		return cache::CatapultCache(std::move(subCaches));
	}

	void CoreSystemCacheFactory::CreateSubCaches(
			const config::BlockchainConfiguration& config,
			std::vector<std::unique_ptr<cache::SubCachePlugin>>& subCaches) {
		CreateSubCaches(config, cache::CacheConfiguration(), subCaches);
	}

	void CoreSystemCacheFactory::CreateSubCaches(
			const config::BlockchainConfiguration& config,
			const cache::CacheConfiguration& cacheConfig,
			std::vector<std::unique_ptr<cache::SubCachePlugin>>& subCaches) {
		using namespace cache;

		auto pConfigHolder = config::CreateMockConfigurationHolder(config);
		subCaches[AccountStateCache::Id] = MakeSubCachePluginWithCacheConfiguration<AccountStateCache, AccountStateCacheStorage>(
				cacheConfig,
				CreateAccountStateCacheOptions(pConfigHolder));

		subCaches[BlockDifficultyCache::Id] = MakeConfigurationFreeSubCachePlugin<BlockDifficultyCache, BlockDifficultyCacheStorage>(pConfigHolder);

		std::string name;
		if(cacheConfig.CacheDatabaseDirectory == "") {
			name = cache::NetworkConfigCache::Name;
		} else {
			name = cacheConfig.CacheDatabaseDirectory+"/"+cache::NetworkConfigCache::Name;
		}
		auto netConfigCacheConfig = CacheConfiguration(cache::NetworkConfigCache::Name, utils::FileSize(), cacheConfig.ShouldStorePatriciaTrees ? PatriciaTreeStorageMode::Enabled : PatriciaTreeStorageMode::Disabled);
		netConfigCacheConfig.ShouldUseCacheDatabase = cacheConfig.ShouldUseCacheDatabase;
		subCaches[NetworkConfigCache::Id] = MakeSubCachePluginWithCacheConfiguration<NetworkConfigCache, NetworkConfigCacheStorage>(netConfigCacheConfig, pConfigHolder);
	}

	// endregion

	cache::CatapultCache CreateEmptyCatapultCache() {
		return CreateEmptyCatapultCache<CoreSystemCacheFactory>(test::MutableBlockchainConfiguration().ToConst());
	}

	cache::CatapultCache CreateEmptyCatapultCache(const config::BlockchainConfiguration& config) {
		return CreateEmptyCatapultCache<CoreSystemCacheFactory>(config);
	}

	cache::CatapultCache CreateEmptyCatapultCache(const cache::CacheConfiguration& cacheConfig) {
		return CreateEmptyCatapultCache(test::MutableBlockchainConfiguration().ToConst(), cacheConfig);
	}

	cache::CatapultCache CreateEmptyCatapultCache(
			const config::BlockchainConfiguration& config,
			const cache::CacheConfiguration& cacheConfig) {
		std::vector<std::unique_ptr<cache::SubCachePlugin>> subCaches(3);
		CoreSystemCacheFactory::CreateSubCaches(config, cacheConfig, subCaches);
		return cache::CatapultCache(std::move(subCaches));
	}

	cache::CatapultCache CreateCatapultCacheWithMarkerAccount() {
		return CreateCatapultCacheWithMarkerAccount(Height(0), test::MutableBlockchainConfiguration().ToConst());
	}

	cache::CatapultCache CreateCatapultCacheWithMarkerAccount(Height height) {
		return CreateCatapultCacheWithMarkerAccount(height, test::MutableBlockchainConfiguration().ToConst());
	}

	cache::CatapultCache CreateCatapultCacheWithMarkerAccount(const config::BlockchainConfiguration& config) {
		return CreateCatapultCacheWithMarkerAccount(Height(0), config);
	}

	cache::CatapultCache CreateCatapultCacheWithMarkerAccount(Height height, const config::BlockchainConfiguration& config) {
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
