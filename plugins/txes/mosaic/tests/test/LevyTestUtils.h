/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once

#include "tests/test/core/BalanceTransfers.h"
#include "src/catapult/model/ResolverContext.h"
#include "src/state/LevyEntry.h"
#include "src/cache/LevyCache.h"
#include "src/cache/LevyCacheStorage.h"
#include "src/cache/MosaicCache.h"
#include "src/cache/MosaicCacheStorage.h"
#include "tests/test/cache/CacheTestUtils.h"
#include "tests/test/other/MutableBlockchainConfiguration.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"

namespace catapult { namespace test {
		
	/// Cache factory for creating a catapult cache containing mosaic and levy cache
	struct LevyCacheFactory {
	private:
		static auto CreateSubCachesWithLevyCache(const config::BlockchainConfiguration& config = config::BlockchainConfiguration::Uninitialized()) {
			auto levyCacheId = cache::LevyCache::Id;
			
			std::vector<std::unique_ptr<cache::SubCachePlugin>> subCaches(levyCacheId + 2);
			auto pConfigHolder = config::CreateMockConfigurationHolder(config);
			subCaches[levyCacheId] = MakeSubCachePlugin<cache::LevyCache, cache::LevyCacheStorage>(pConfigHolder);
			
			auto mosaicCacheId = cache::MosaicCache::Id;
			subCaches[mosaicCacheId] = MakeSubCachePlugin<cache::MosaicCache, cache::MosaicCacheStorage>();
			return subCaches;
		}
	
	public:
		/// Creates an empty catapult cache.
		static cache::CatapultCache Create() {
			auto config = test::MutableBlockchainConfiguration().ToConst();
			auto subCaches = CreateSubCachesWithLevyCache();
			CoreSystemCacheFactory::CreateSubCaches(config, subCaches);
			return cache::CatapultCache(std::move(subCaches));
		}
		
		/// Creates an empty catapult cache around \a config.
		static cache::CatapultCache Create(const config::BlockchainConfiguration& config) {
			auto subCaches = CreateSubCachesWithLevyCache(config);
			CoreSystemCacheFactory::CreateSubCaches(config, subCaches);
			return cache::CatapultCache(std::move(subCaches));
		}
	};
		
	/// creates a valid mosaic levy model
	state::LevyEntryData CreateValidMosaicLevy();
	
	/// creates a valid levy entry for cache
	state::LevyEntry CreateLevyEntry(bool withLevy, bool withHistory);
	
	/// create a levy entry with history parameters
	state::LevyEntry CreateLevyEntry(const MosaicId& mosaicId, state::LevyEntryData& levy,
		bool withLevy, bool withHistory, size_t historyCount = 1, const Height& baseHeight = Height(100));
	
	/// create levy entry using mosaicId and amount fee
	state::LevyEntryData CreateLevyEntry(const MosaicId& mosaicId, Amount amount);
	
	/// converts percentage to fix point amount
	Amount CreateMosaicLevyFeePercentile(float percentage);
	
	/// Add information to mosaic and levy cache
	void AddMosaicWithLevy(cache::CatapultCacheDelta& cache, MosaicId id, Height height, state::LevyEntryData levy);
	
	/// Add information to mosaic and levy cache with owner of mosaic
	void AddMosaicWithLevy(cache::CatapultCacheDelta& cache, MosaicId id, Height height, state::LevyEntryData levy, const Key& owner);
	
	/// region levy test assert
	void AssertLevy(const state::LevyEntryData& rhs, const state::LevyEntryData& lsh);
	
	void AssertLevy(const state::LevyEntryData& rhs, const model::MosaicLevyRaw& raw, model::ResolverContext resolver);
	
	state::LevyEntry& GetLevyEntryFromCache(cache::CatapultCacheDelta& cache, const MosaicId& mosaicId);
	
	/// create mosaic balance from mosaicId and amount
	test::BalanceTransfers CreateMosaicBalance(MosaicId id, Amount amount);
		
	/// create Mosaic config with levy
	config::BlockchainConfiguration CreateMosaicConfigWithLevy(bool levyEnabled);
}}
