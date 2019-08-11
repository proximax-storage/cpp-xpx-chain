/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/state/CatapultConfigEntry.h"
#include "tests/TestHarness.h"

namespace catapult { namespace state {

#define TEST_CLASS CatapultConfigEntryTests

	TEST(TEST_CLASS, CanCreateCatapultConfigEntry) {
		// Act:
		auto height = Height{1};
		auto entry = CatapultConfigEntry(height);

		// Assert:
		EXPECT_EQ(height, entry.height());
	}

	TEST(TEST_CLASS, CanSetHeight) {
		// Arrange:
		auto height = Height{1};
		auto entry = CatapultConfigEntry(height);

		// Act:
		entry.setHeight(Height{12});

		// Assert:
		EXPECT_EQ(12, entry.height().unwrap());
	}

	TEST(TEST_CLASS, CanSetBlockChainConfig) {
		// Arrange:
		auto entry = CatapultConfigEntry();

		// Act:
		entry.setBlockChainConfig("aaa");

		// Assert:
		EXPECT_EQ_MEMORY("aaa", entry.blockChainConfig().data(), 3);
	}

	TEST(TEST_CLASS, CanSetSupportedEntityVersions) {
		// Arrange:
		auto entry = CatapultConfigEntry();

		// Act:
		entry.setSupportedEntityVersions("bbbb");

		// Assert:
		EXPECT_EQ_MEMORY("bbbb", entry.supportedEntityVersions().data(), 4);
	}
}}
