/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/cache/BlockchainUpgradeCacheStorage.h"
#include "src/cache/BlockchainUpgradeCache.h"
#include "tests/TestHarness.h"

namespace catapult { namespace cache {

#define TEST_CLASS BlockchainUpgradeCacheStorageTests

	TEST(TEST_CLASS, CanLoadValueIntoCache) {
		// Arrange: create a random value to insert
		state::BlockchainUpgradeEntry originalEntry(Height{1}, BlockchainVersion{1});

		// Act:
		BlockchainUpgradeCache cache(CacheConfiguration{});
		auto delta = cache.createDelta(Height{1});
		BlockchainUpgradeCacheStorage::LoadInto(originalEntry, *delta);
		cache.commit();

		// Assert: the cache contains the value
		auto view = cache.createView(Height{1});
		EXPECT_EQ(1u, view->size());
		ASSERT_TRUE(view->contains(originalEntry.height()));
		const auto& loadedEntry = view->find(originalEntry.height()).get();

		// - the loaded cache value is correct
		EXPECT_EQ(originalEntry.height(), loadedEntry.height());
		EXPECT_EQ(originalEntry.blockChainVersion(), loadedEntry.blockChainVersion());
	}
}}
