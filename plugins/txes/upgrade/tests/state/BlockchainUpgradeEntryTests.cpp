/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/state/BlockchainUpgradeEntry.h"
#include "tests/TestHarness.h"

namespace catapult { namespace state {

#define TEST_CLASS BlockchainUpgradeEntryTests

	TEST(TEST_CLASS, CanCreateBlockchainUpgradeEntry) {
		// Act:
		auto height = Height{1};
		auto entry = BlockchainUpgradeEntry(height);

		// Assert:
		EXPECT_EQ(height, entry.height());
	}

	TEST(TEST_CLASS, CanSetHeight) {
		// Arrange:
		auto height = Height{1};
		auto entry = BlockchainUpgradeEntry(height);

		// Act:
		entry.setHeight(Height{12});

		// Assert:
		EXPECT_EQ(12, entry.height().unwrap());
	}

	TEST(TEST_CLASS, CanSetBlockchainVersion) {
		// Arrange:
		auto entry = BlockchainUpgradeEntry();

		// Act:
		entry.setBlockchainVersion(BlockchainVersion{5});

		// Assert:
		EXPECT_EQ(5, entry.blockChainVersion().unwrap());
	}
}}
