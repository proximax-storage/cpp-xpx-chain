/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/state/CatapultUpgradeEntry.h"
#include "tests/TestHarness.h"

namespace catapult { namespace state {

#define TEST_CLASS CatapultUpgradeEntryTests

	TEST(TEST_CLASS, CanCreateCatapultUpgradeEntry) {
		// Act:
		auto height = Height{1};
		auto entry = CatapultUpgradeEntry(height);

		// Assert:
		EXPECT_EQ(height, entry.height());
	}

	TEST(TEST_CLASS, CanSetHeight) {
		// Arrange:
		auto height = Height{1};
		auto entry = CatapultUpgradeEntry(height);

		// Act:
		entry.setHeight(Height{12});

		// Assert:
		EXPECT_EQ(12, entry.height().unwrap());
	}

	TEST(TEST_CLASS, CanSetCatapultVersion) {
		// Arrange:
		auto entry = CatapultUpgradeEntry();

		// Act:
		entry.setCatapultVersion(CatapultVersion{5});

		// Assert:
		EXPECT_EQ(5, entry.catapultVersion().unwrap());
	}
}}
