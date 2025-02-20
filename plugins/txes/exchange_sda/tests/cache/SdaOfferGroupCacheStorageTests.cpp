/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/cache/SdaOfferGroupCacheStorage.h"
#include "src/cache/SdaOfferGroupCache.h"
#include "tests/test/SdaExchangeTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace cache {

#define TEST_CLASS SdaOfferGroupCacheStorageTests

	TEST(TEST_CLASS, CanLoadValueIntoCache) {
		// Arrange: create a random value to insert
		auto originalEntry = test::CreateSdaOfferGroupEntry();

		// Act:
		SdaOfferGroupCache cache(CacheConfiguration{}, config::CreateMockConfigurationHolder());
		auto delta = cache.createDelta(Height{1});
		SdaOfferGroupCacheStorage::LoadInto(originalEntry, *delta);
		cache.commit();

		// Assert: the cache contains the value
		auto view = cache.createView(Height{1});
		EXPECT_EQ(1u, view->size());
		ASSERT_TRUE(view->contains(originalEntry.groupHash()));
		const auto& loadedEntry = view->find(originalEntry.groupHash()).get();

		// - the loaded cache value is correct
		test::AssertEqualSdaOfferGroupData(originalEntry, loadedEntry);
	}
}}
