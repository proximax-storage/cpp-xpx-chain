/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "tests/test/StorageTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace cache {

#define TEST_CLASS BootKeyReplicatorCacheStorageTests

	TEST(TEST_CLASS, CanLoadValueIntoCache) {
		// Arrange: create a random value to insert
		auto originalEntry = test::CreateBootKeyReplicatorEntry();

		// Act:
		BootKeyReplicatorCache cache(CacheConfiguration{}, config::CreateMockConfigurationHolder());
		auto delta = cache.createDelta(Height{1});
		BootKeyReplicatorCacheStorage::LoadInto(originalEntry, *delta);
		cache.commit();

		// Assert: the cache contains the value
		auto view = cache.createView(Height{1});
		EXPECT_EQ(1u, view->size());
		ASSERT_TRUE(view->contains(originalEntry.nodeBootKey()));
		auto iter = view->find(originalEntry.nodeBootKey());
		const auto& loadedEntry = iter.get();

		// - the loaded cache value is correct
		EXPECT_EQ(originalEntry.nodeBootKey(), loadedEntry.nodeBootKey());
		EXPECT_EQ(originalEntry.replicatorKey(), loadedEntry.replicatorKey());
	}
}}
