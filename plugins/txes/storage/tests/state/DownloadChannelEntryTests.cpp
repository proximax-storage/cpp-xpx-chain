/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/state/DownloadChannelEntry.h"
#include "tests/TestHarness.h"

namespace catapult { namespace state {

#define TEST_CLASS DownloadChannelEntryTests

    TEST(TEST_CLASS, DownloadChannelEntry) {
        // Act:
        auto id = test::GenerateRandomByteArray<Hash256>();
        auto entry = DownloadChannelEntry(id);

        // Assert:
        EXPECT_EQ(id, entry.id());
    }

    TEST(TEST_CLASS, CanAccessConsumer) {
        // Arrange:
        auto consumer = test::GenerateRandomByteArray<Key>();
        auto entry = DownloadChannelEntry(Hash256());

        // Sanity:
        ASSERT_EQ(Key(), entry.consumer());

        // Act:
        entry.setConsumer(consumer);

        // Assert:
        EXPECT_EQ(consumer, entry.consumer());
    }
}}