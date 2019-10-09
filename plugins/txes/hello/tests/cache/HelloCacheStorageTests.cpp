/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/cache/HelloCacheStorage.h"
#include "src/cache/HelloCache.h"
#include "tests/TestHarness.h"

namespace catapult { namespace cache {

#define TEST_CLASS HelloCacheStorageTests

        TEST(TEST_CLASS, CanLoadValueIntoCache) {
            // Arrange: create a random value to insert
            uint16_t count = 10;
            auto key = test::GenerateKeys(1);

            state::HelloEntry originalEntry(key[0], count);

            // Act:
            HelloCache cache(CacheConfiguration{});
            auto delta = cache.createDelta();
            HelloCacheStorage::LoadInto(originalEntry, *delta);
            cache.commit();

            // Assert: the cache contains the value
            auto view = cache.createView();
            EXPECT_EQ(1u, view->size());
            ASSERT_TRUE(view->contains(originalEntry.messageCount()));
            const auto& loadedEntry = view->find(originalEntry.messageCount());

            // - the loaded cache value is correct
            EXPECT_EQ(originalEntry.messageCount(), loadedEntry.messageCount());
            EXPECT_EQ(originalEntry.key(), loadedEntry.key());
        }
    }}
