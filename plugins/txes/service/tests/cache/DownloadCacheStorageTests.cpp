/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/cache/DownloadCacheStorage.h"
#include "src/cache/DownloadCache.h"
#include "tests/test/ServiceTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace cache {

#define TEST_CLASS DownloadCacheStorageTests

	TEST(TEST_CLASS, CanLoadValueIntoCache) {
		// Arrange: create a random value to insert
		auto originalEntry = test::CreateDownloadEntry();

		// Act:
		DownloadCache cache(CacheConfiguration{}, config::CreateMockConfigurationHolder());
		auto delta = cache.createDelta(Height{1});
		DownloadCacheStorage::LoadInto(originalEntry, *delta);
		cache.commit();

		// Assert: the cache contains the value
		auto view = cache.createView(Height{1});
		EXPECT_EQ(1u, view->size());
		ASSERT_TRUE(view->contains(originalEntry.driveKey()));
		const auto& loadedEntry = view->find(originalEntry.driveKey()).get();

		// - the loaded cache value is correct
		test::AssertEqualDownloadData(originalEntry, loadedEntry);
	}
}}
