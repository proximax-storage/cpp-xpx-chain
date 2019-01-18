/**
*** Copyright 2018 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/cache/ReputationCacheStorage.h"
#include "src/cache/ReputationCache.h"
#include "tests/TestHarness.h"

namespace catapult { namespace cache {

#define TEST_CLASS ReputationCacheStorageTests

	TEST(TEST_CLASS, CanLoadValueIntoCache) {
		// Arrange: create a random value to insert
		state::ReputationEntry originalEntry(test::GenerateRandomData<Key_Size>());
		originalEntry.setPositiveInteractions(Reputation{12});
		originalEntry.setNegativeInteractions(Reputation{34});

		// Act:
		ReputationCache cache(CacheConfiguration{});
		auto delta = cache.createDelta();
		ReputationCacheStorage::LoadInto(originalEntry, *delta);
		cache.commit();

		// Assert: the cache contains the value
		auto view = cache.createView();
		EXPECT_EQ(1u, view->size());
		ASSERT_TRUE(view->contains(originalEntry.key()));
		const auto& loadedEntry = view->find(originalEntry.key()).get();

		// - the loaded cache value is correct
		EXPECT_EQ(originalEntry.key(), loadedEntry.key());
		EXPECT_EQ(originalEntry.positiveInteractions(), loadedEntry.positiveInteractions());
		EXPECT_EQ(originalEntry.negativeInteractions(), loadedEntry.negativeInteractions());
	}
}}
