/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/cache/CatapultConfigCacheStorage.h"
#include "src/cache/CatapultConfigCache.h"
#include "tests/TestHarness.h"

namespace catapult { namespace cache {

#define TEST_CLASS CatapultConfigCacheStorageTests

	TEST(TEST_CLASS, CanLoadValueIntoCache) {
		// Arrange: create a random value to insert
		state::CatapultConfigEntry originalEntry(Height{1}, "blockChainConfig", "supportedEntityVersions");

		// Act:
		CatapultConfigCache cache(CacheConfiguration{});
		auto delta = cache.createDelta(Height{1});
		CatapultConfigCacheStorage::LoadInto(originalEntry, *delta);
		cache.commit();

		// Assert: the cache contains the value
		auto view = cache.createView(Height{1});
		EXPECT_EQ(1u, view->size());
		ASSERT_TRUE(view->contains(originalEntry.height()));
		const auto& loadedEntry = view->find(originalEntry.height()).get();

		// - the loaded cache value is correct
		EXPECT_EQ(originalEntry.height(), loadedEntry.height());
		EXPECT_EQ(originalEntry.blockChainConfig(), loadedEntry.blockChainConfig());
		EXPECT_EQ(originalEntry.supportedEntityVersions(), loadedEntry.supportedEntityVersions());
	}
}}
