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

#pragma once
#include "src/cache/MosaicCache.h"
#include "src/cache/MosaicCacheStorage.h"
#include "tests/test/cache/CacheTestUtils.h"
#include "tests/test/other/MutableBlockchainConfiguration.h"

namespace catapult { namespace test {

	/// Cache factory for creating a catapult cache containing at least the mosaic cache.
	struct MosaicCacheFactory {
	private:
		static auto CreateSubCachesWithMosaicCache() {
			auto cacheId = cache::MosaicCache::Id;
			std::vector<std::unique_ptr<cache::SubCachePlugin>> subCaches(cacheId + 1);
			subCaches[cacheId] = MakeSubCachePlugin<cache::MosaicCache, cache::MosaicCacheStorage>();
			return subCaches;
		}

	public:
		/// Creates an empty catapult cache.
		static cache::CatapultCache Create() {
			return Create(test::MutableBlockchainConfiguration().ToConst());
		}

		/// Creates an empty catapult cache around \a config.
		static cache::CatapultCache Create(const config::BlockchainConfiguration& config) {
			auto subCaches = CreateSubCachesWithMosaicCache();
			CoreSystemCacheFactory::CreateSubCaches(config, subCaches);
			return cache::CatapultCache(std::move(subCaches));
		}
	};

	/// Adds mosaic \a id to the mosaic cache in \a cache at \a height with \a duration and \a supply.
	void AddMosaic(cache::CatapultCacheDelta& cache, MosaicId id, Height height, BlockDuration duration, Amount supply);

	/// Adds mosaic \a id to the mosaic cache in \a cache at \a height with \a duration and \a owner.
	void AddMosaic(cache::CatapultCacheDelta& cache, MosaicId id, Height height, BlockDuration duration, const Key& owner);

	/// Adds eternal mosaic \a id to the mosaic cache in \a cache at \a height.
	void AddEternalMosaic(cache::CatapultCacheDelta& cache, MosaicId id, Height height);

	/// Adds eternal mosaic \a id to the mosaic cache in \a cache at \a height with \a owner.
	void AddEternalMosaic(cache::CatapultCacheDelta& cache, MosaicId id, Height height, const Key& owner);

	/// Adds account \a owner to the account state cache in \a cache with \a amount units of mosaic \a id.
	void AddMosaicOwner(cache::CatapultCacheDelta& cache, MosaicId id, const Key& owner, Amount amount);

	/// Adds account \a owner to the account state cache in \a cache with \a amount units of mosaic \a id.
	void AddMosaicOwner(cache::CatapultCacheDelta& cache, MosaicId id, Address& owner, Amount amount);

		/// Asserts that \a cache exactly contains the mosaic ids in \a expectedIds.
	void AssertCacheContents(const cache::MosaicCache& cache, std::initializer_list<MosaicId::ValueType> expectedIds);

	/// Asserts that \a cache exactly contains the mosaic ids in \a expectedIds.
	void AssertCacheContents(const cache::MosaicCacheView& cache, std::initializer_list<MosaicId::ValueType> expectedIds);

	/// Asserts that \a cache exactly contains the mosaic ids in \a expectedIds.
	void AssertCacheContents(const cache::MosaicCacheDelta& cache, std::initializer_list<MosaicId::ValueType> expectedIds);

	/// Adds mosaic \a id to the mosaic cache in \a cache at \a height with \a mosaic levy
	void AddMosaicWithLevy(cache::CatapultCacheDelta& cache, MosaicId id, Height height, BlockDuration duration, model::MosaicLevy levy);
	
	/// Adds mosaic \a id to the mosaic cache in \a cache at \a height with \a mosaic levy and owner
	void AddMosaicWithLevy(cache::CatapultCacheDelta& cache, MosaicId id, Height height, BlockDuration duration, model::MosaicLevy levy, const Key& owner);

}}
