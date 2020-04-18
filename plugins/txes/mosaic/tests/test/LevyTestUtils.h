/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/model/MosaicLevy.h"
#include "src/cache/LevyCache.h"
#include "src/cache/LevyCacheStorage.h"
#include "src/cache/MosaicCache.h"
#include "src/cache/MosaicCacheStorage.h"
#include "tests/test/cache/CacheTestUtils.h"
#include "tests/test/other/MutableBlockchainConfiguration.h"

namespace catapult { namespace test {
		
	/// Cache factory for creating a catapult cache containing mosaic and levy cache
	struct LevyCacheFactory {
	private:
		static auto CreateSubCachesWithLevyCache() {
			auto levyCacheId = cache::LevyCache::Id;
			
			std::vector<std::unique_ptr<cache::SubCachePlugin>> subCaches(levyCacheId + 2);
			subCaches[levyCacheId] = MakeSubCachePlugin<cache::LevyCache, cache::LevyCacheStorage>();
			
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
	};
		
	/// creates a valid mosaic levy model
	model::MosaicLevy CreateValidMosaicLevy();
	
	/// converts percentage to fix point amount
	Amount CreateMosaicLevyFeePercentile(float percentage);
	
	/// Add information to mosaic and levy cache
	void AddMosaicWithLevy(cache::CatapultCacheDelta& cache, MosaicId id, Height height, model::MosaicLevy levy);
	
	/// Add information to mosaic and levy cache with owner of mosaic
	void AddMosaicWithLevy(cache::CatapultCacheDelta& cache, MosaicId id, Height height, model::MosaicLevy levy, const Key& owner);
}}
