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

#include "catapult/cache/CatapultCacheBuilder.h"
#include "tests/test/cache/SimpleCache.h"
#include "tests/TestHarness.h"

namespace catapult { namespace cache {

#define TEST_CLASS CatapultCacheBuilderTests

	namespace {
		template<size_t CacheId>
		void AddSubCacheWithId(CatapultCacheBuilder& builder) {
			builder.addSubCache<test::SimpleCacheStorageTraits>(std::make_unique<test::SimpleCacheT<CacheId>>());
		}

		size_t GetNumSubCaches(const CatapultCache& cache) {
			return cache.storages().size();
		}
	}

	TEST(TEST_CLASS, CanCreateEmptyCatapultCache) {
		// Arrange:
		CatapultCacheBuilder builder;

		// Act:
		auto cache = builder.buildCache();

		// Assert:
		EXPECT_EQ(0u, GetNumSubCaches(cache));
	}

	TEST(TEST_CLASS, CanCreateCatapultCacheWithSingleSubCache) {
		// Arrange:
		CatapultCacheBuilder builder;
		AddSubCacheWithId<2>(builder);

		// Act:
		auto cache = builder.buildCache();

		// Assert:
		EXPECT_EQ(1u, GetNumSubCaches(cache));
	}

	TEST(TEST_CLASS, CanCreateCatapultCacheWithMultipleSubCaches) {
		// Arrange:
		CatapultCacheBuilder builder;
		AddSubCacheWithId<2>(builder);
		AddSubCacheWithId<6>(builder);
		AddSubCacheWithId<4>(builder);

		// Act:
		auto cache = builder.buildCache();

		// Assert:
		EXPECT_EQ(3u, GetNumSubCaches(cache));
	}

	TEST(TEST_CLASS, CannotAddMultipleSubCachesWithSameId) {
		// Arrange:
		CatapultCacheBuilder builder;
		AddSubCacheWithId<2>(builder);
		AddSubCacheWithId<6>(builder);
		AddSubCacheWithId<4>(builder);

		// Act + Assert:
		EXPECT_THROW(AddSubCacheWithId<6>(builder), catapult_invalid_argument);
	}
}}
