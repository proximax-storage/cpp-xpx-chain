/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "tests/test/CommitteeTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace cache {

#define TEST_CLASS CommitteeCacheStorageTests

	TEST(TEST_CLASS, CanLoadValueIntoCache) {
		// Arrange: create a random value to insert
		auto originalEntry = test::CreateCommitteeEntry();

		// Act:
		CommitteeCache cache(CacheConfiguration{}, std::make_shared<cache::CommitteeAccountCollector>(), config::CreateMockConfigurationHolder());
		auto delta = cache.createDelta(Height{1});
		CommitteeCacheStorage::LoadInto(originalEntry, *delta);
		cache.commit();

		// Assert: the cache contains the value
		auto view = cache.createView(Height{1});
		EXPECT_EQ(1u, view->size());
		ASSERT_TRUE(view->contains(originalEntry.key()));
		const auto& loadedEntry = view->find(originalEntry.key()).get();

		// - the loaded cache value is correct
		test::AssertEqualCommitteeEntry(originalEntry, loadedEntry);
	}
}}
