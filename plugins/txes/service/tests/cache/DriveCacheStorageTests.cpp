/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/cache/DriveCacheStorage.h"
#include "src/cache/DriveCache.h"
#include "tests/test/ServiceTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace cache {

#define TEST_CLASS DriveCacheStorageTests

	TEST(TEST_CLASS, CanLoadValueIntoCache) {
		// Arrange: create a random value to insert
		auto originalEntry = test::CreateDriveEntry();

		// Act:
		DriveCache cache(CacheConfiguration{}, config::CreateMockConfigurationHolder());
		auto delta = cache.createDelta(Height{1});
		DriveCacheStorage::LoadInto(originalEntry, *delta);
		cache.commit();

		// Assert: the cache contains the value
		auto view = cache.createView(Height{1});
		EXPECT_EQ(1u, view->size());
		ASSERT_TRUE(view->contains(originalEntry.key()));
		const auto& loadedEntry = view->find(originalEntry.key()).get();

		// - the loaded cache value is correct
		test::AssertEqualDriveData(originalEntry, loadedEntry);
	}
}}
