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
	cache::CatapultCache LockFundCacheFactory::Create(const config::BlockchainConfiguration& config) {
		std::vector<std::unique_ptr<cache::SubCachePlugin>> subCaches(4);
		CreateSubCaches(config, cache::CacheConfiguration(), subCaches);
		return cache::CatapultCache(std::move(subCaches));
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
