/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/cache/HelloCacheStorage.h"
#include "src/cache/HelloCache.h"
#include "tests/TestHarness.h"
#include "tests/test/HelloTestUtils.h"
#include "src/state/HelloEntry.h"

namespace catapult { namespace cache {

#define TEST_CLASS HelloCacheStorageTests

        TEST(TEST_CLASS, CanLoadValueIntoCache) {
#if 1
            // Arrange: create a random value to insert
            uint16_t count = 10;
            auto key = test::GenerateKeys(1);

            state::HelloEntry originalEntry(key[0], count);

            // Act:
            HelloCache cache(CacheConfiguration{});
            /*8catapult::cache::LockedCacheDelta<catapult::cache::HelloCacheDelta>*/
            auto delta = cache.createDelta(Height{1});
            HelloCacheStorage::LoadInto(originalEntry, *delta);
            cache.commit();

            // Assert: the cache contains the value
            auto view = cache.createView(Height{1});
            EXPECT_EQ(1u, view->size());
            ASSERT_TRUE(view->contains(originalEntry.key()));
            const auto& loadedEntry = view->find(originalEntry.key()).get();

            // - the loaded cache value is correct
            EXPECT_EQ(originalEntry.messageCount(), loadedEntry.messageCount());
#endif
        }
    }}
