/**
*** FOR TRAINING PURPOSES ONLY
**/


#include "src/state/HelloEntry.h"
#include "tests/TestHarness.h"
#include "tests/test/HelloTestUtils.h"

namespace catapult { namespace state {

#define TEST_CLASS BlockchainUpgradeEntryTests

        TEST(TEST_CLASS, CanCreateWithKeysCount) {
            // Act:
            uint16_t count = 10;
            auto key = test::GenerateKeys(1);
            auto entry = HelloEntry(key[0], count);

            // Assert:
            EXPECT_EQ(count, entry.messageCount());
            EXPECT_EQ(key[0], entry.key());
        }
    }}
