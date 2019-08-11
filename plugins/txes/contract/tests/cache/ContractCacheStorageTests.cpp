/**
*** Copyright 2018 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/cache/ContractCacheStorage.h"
#include "src/cache/ContractCache.h"
#include "tests/TestHarness.h"

namespace catapult { namespace cache {

#define TEST_CLASS ContractCacheStorageTests

	TEST(TEST_CLASS, CanLoadValueIntoCache) {
		// Arrange: create a random value to insert
		state::ContractEntry originalEntry(test::GenerateRandomByteArray<Key>());
		originalEntry.setDuration(BlockDuration(20));
		originalEntry.setStart(Height(12));
		originalEntry.pushHash(test::GenerateRandomByteArray<Hash256>(), Height(12));
		originalEntry.pushHash(test::GenerateRandomByteArray<Hash256>(), Height(13));
		originalEntry.executors() = { test::GenerateRandomByteArray<Key>(), test::GenerateRandomByteArray<Key>(), test::GenerateRandomByteArray<Key>() };
		originalEntry.customers() = { test::GenerateRandomByteArray<Key>(), test::GenerateRandomByteArray<Key>(), test::GenerateRandomByteArray<Key>() };
		originalEntry.verifiers() = { test::GenerateRandomByteArray<Key>(), test::GenerateRandomByteArray<Key>(), test::GenerateRandomByteArray<Key>() };

		// Act:
		ContractCache cache(CacheConfiguration{});
		auto delta = cache.createDelta(Height(0));
		ContractCacheStorage::LoadInto(originalEntry, *delta);
		cache.commit();

		// Assert: the cache contains the value
		auto view = cache.createView(Height(0));
		EXPECT_EQ(1u, view->size());
		ASSERT_TRUE(view->contains(originalEntry.key()));
		const auto& loadedEntry = view->find(originalEntry.key()).get();

		// - the loaded cache value is correct
		EXPECT_EQ(originalEntry.key(), loadedEntry.key());
		EXPECT_EQ(originalEntry.start(), loadedEntry.start());
		EXPECT_EQ(originalEntry.duration(), loadedEntry.duration());
		EXPECT_EQ(originalEntry.hashes(), loadedEntry.hashes());
		EXPECT_EQ(originalEntry.executors(), loadedEntry.executors());
		EXPECT_EQ(originalEntry.customers(), loadedEntry.customers());
		EXPECT_EQ(originalEntry.verifiers(), loadedEntry.verifiers());
	}
}}
