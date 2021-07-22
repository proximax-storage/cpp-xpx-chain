/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/state/ReplicatorEntry.h"
#include "tests/TestHarness.h"

namespace catapult { namespace state {

#define TEST_CLASS ReplicatorEntryTests

    TEST(TEST_CLASS, ReplicatorEntry) {
        // Act:
        auto key = test::GenerateRandomByteArray<Key>();
        auto entry = ReplicatorEntry(key);

        // Assert:
        EXPECT_EQ(key, entry.key());
    }

    TEST(TEST_CLASS, CanAccessCapacity) {
        // Arrange:
        auto capacity = Amount(10);
        auto entry = ReplicatorEntry(Key());

        // Sanity:
        ASSERT_EQ(Amount(0), entry.capacity());

        // Act:
        entry.setCapacity(capacity);

        // Assert:
        EXPECT_EQ(capacity, entry.capacity());
    }

    TEST(TEST_CLASS, CanAccessDrives) {
        // Arrange:
        utils::KeySet drives = { test::GenerateRandomByteArray<Key>(), test::GenerateRandomByteArray<Key>() };
        auto entry = ReplicatorEntry(Key());

        // Sanity:
        ASSERT_TRUE(entry.drives().empty());

        // Act:
        entry.drives() = drives;

        // Assert:
        ASSERT_EQ(2, entry.drives().size());
        EXPECT_EQ(drives, entry.drives());
    }

}}