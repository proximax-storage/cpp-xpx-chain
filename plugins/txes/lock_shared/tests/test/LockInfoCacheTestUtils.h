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
#include "plugins/txes/lock_shared/src/state/LockInfo.h"
#include "tests/test/cache/CacheTestUtils.h"
#include "tests/test/other/MutableBlockchainConfiguration.h"
#include "tests/TestHarness.h"

namespace catapult { namespace test {

	/// Creates \a count lock infos with increasing heights.
	template<typename TLockInfoTraits>
	auto CreateLockInfos(size_t count) {
		std::vector<typename TLockInfoTraits::ValueType> lockInfos;
		for (auto i = 0u; i < count; ++i) {
			auto status = (0 == i % 2) ? state::LockStatus::Used : state::LockStatus::Unused;
			lockInfos.emplace_back(TLockInfoTraits::CreateLockInfo(Height((i + 1) * 10), status));
		}

		return lockInfos;
	}

	/// Asserts that \a lhs and \a rhs are equal.
	inline void AssertEqualLockInfo(const state::LockInfo& lhs, const state::LockInfo& rhs) {
		EXPECT_EQ(lhs.Account, rhs.Account);
		EXPECT_EQ(lhs.Mosaics.size(), rhs.Mosaics.size());
		for (const auto& pair:  lhs.Mosaics) {
			EXPECT_EQ(pair.second, rhs.Mosaics.at(pair.first));
		}
		EXPECT_EQ(lhs.Height, rhs.Height);
		EXPECT_EQ(lhs.Status, rhs.Status);
	}

	/// Asserts that \a cache contains exactly all expected lock infos (\a expectedLockInfos).
	template<typename TLockInfoTraits>
	void AssertCacheContents(
			const typename TLockInfoTraits::CacheDeltaType& cache,
			const std::vector<typename TLockInfoTraits::ValueType>& expectedLockInfos) {
		EXPECT_EQ(expectedLockInfos.size(), cache.size());

		for (const auto& expectedLockInfo : expectedLockInfos) {
			auto key = TLockInfoTraits::ToKey(expectedLockInfo);
			ASSERT_TRUE(cache.contains(key));

			const auto& lockInfo = cache.get(key);
			TLockInfoTraits::AssertEqual(expectedLockInfo, lockInfo);
		}
	}

	/// Basic lock info cache factory.
	template<typename TCacheTraits, typename TCacheStorage>
	struct LockInfoCacheFactory {
	public:
		using LockInfoCache = typename TCacheTraits::CacheType;
		using LockInfoCacheStorage = TCacheStorage;

	private:
		static auto CreateSubCachesWithLockHashCache() {
			auto cacheId = LockInfoCache::Id;
			std::vector<std::unique_ptr<cache::SubCachePlugin>> subCaches(cacheId + 1);
			subCaches[cacheId] = MakeSubCachePlugin<LockInfoCache, LockInfoCacheStorage>();
			return subCaches;
		}

	public:
		/// Creates an empty catapult cache.
		static cache::CatapultCache Create() {
			return Create(test::MutableBlockchainConfiguration().ToConst());
		}

		/// Creates an empty catapult cache around \a config.
		static cache::CatapultCache Create(const config::BlockchainConfiguration& config) {
			auto subCaches = CreateSubCachesWithLockHashCache();
			CoreSystemCacheFactory::CreateSubCaches(config, subCaches);
			return cache::CatapultCache(std::move(subCaches));
		}
	};
}}
