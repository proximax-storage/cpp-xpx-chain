/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/cache/NetworkConfigCacheStorage.h"
#include "src/cache/NetworkConfigCache.h"
#include "tests/test/NetworkConfigTestUtils.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "tests/TestHarness.h"

namespace catapult { namespace cache {

#define TEST_CLASS NetworkConfigCacheStorageTests

	TEST(TEST_CLASS, CanLoadValueIntoCache) {
		// Arrange: create a random value to insert
		state::NetworkConfigEntry originalEntry(Height{1}, test::networkConfig(), test::supportedVersions());

		// Act:
		NetworkConfigCache cache(CacheConfiguration{}, config::CreateMockConfigurationHolder());
		auto delta = cache.createDelta(Height{1});
		NetworkConfigCacheStorage::LoadInto(originalEntry, *delta);
		cache.commit();

		// Assert: the cache contains the value
		auto view = cache.createView(Height{1});
		EXPECT_EQ(1u, view->size());
		ASSERT_TRUE(view->contains(originalEntry.height()));
		const auto& loadedEntry = view->find(originalEntry.height()).get();

		// - the loaded cache value is correct
		EXPECT_EQ(originalEntry.height(), loadedEntry.height());
		EXPECT_EQ(originalEntry.networkConfig(), loadedEntry.networkConfig());
		EXPECT_EQ(originalEntry.supportedEntityVersions(), loadedEntry.supportedEntityVersions());
	}
}}
