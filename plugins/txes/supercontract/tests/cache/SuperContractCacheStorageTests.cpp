/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/cache/SuperContractCacheStorage.h"
#include "src/cache/SuperContractCache.h"
#include "tests/test/SuperContractTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace cache {

#define TEST_CLASS SuperContractCacheStorageTests

	TEST(TEST_CLASS, CanLoadValueIntoCache) {
		// Arrange: create a random value to insert
		auto originalEntry = test::CreateSuperContractEntry();

		// Act:
		SuperContractCache cache(CacheConfiguration{}, config::CreateMockConfigurationHolder());
		auto delta = cache.createDelta(Height{1});
		SuperContractCacheStorage::LoadInto(originalEntry, *delta);
		cache.commit();

		// Assert: the cache contains the value
		auto view = cache.createView(Height{1});
		EXPECT_EQ(1u, view->size());
		ASSERT_TRUE(view->contains(originalEntry.key()));
		const auto& loadedEntry = view->find(originalEntry.key()).get();

		// - the loaded cache value is correct
		test::AssertEqualSuperContractData(originalEntry, loadedEntry);
	}
}}
