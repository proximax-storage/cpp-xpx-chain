/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/state/BootKeyReplicatorEntry.h"
#include "tests/TestHarness.h"

namespace catapult { namespace state {

#define TEST_CLASS BootKeyReplicatorEntryTests

    TEST(TEST_CLASS, BootKeyReplicatorEntry) {
        // Act:
        auto nodeBootKey = test::GenerateRandomByteArray<Key>();
        auto replicatorKey = test::GenerateRandomByteArray<Key>();
        auto entry = BootKeyReplicatorEntry(nodeBootKey, replicatorKey);

        // Assert:
        EXPECT_EQ(1, entry.version());
        EXPECT_EQ(nodeBootKey, entry.nodeBootKey());
        EXPECT_EQ(replicatorKey, entry.replicatorKey());
    }
}}