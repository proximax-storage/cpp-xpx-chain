/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/state/CommitteeEntry.h"
#include "tests/TestHarness.h"

namespace catapult { namespace state {

#define TEST_CLASS CommitteeEntryTests

	TEST(TEST_CLASS, CanCreateCommitteeEntry) {
		// Act:
		auto key = test::GenerateRandomByteArray<Key>();
		auto owner = test::GenerateRandomByteArray<Key>();
		auto entry = CommitteeEntry(key, owner, Height(), Importance(), false, 0.0, 0.0, Height(10));

		// Assert:
		EXPECT_EQ(key, entry.key());
		EXPECT_EQ(owner, entry.owner());
		EXPECT_EQ(Height(10), entry.disabledHeight());
	}

	TEST(TEST_CLASS, CanSetLastSigningBlockHeight) {
		// Arrange:
		auto entry = CommitteeEntry(Key(), Key(), Height(5), Importance(), false, 0.0, 0.0);

		// Sanity check:
		EXPECT_EQ(Height(5), entry.lastSigningBlockHeight());

		// Act:
		entry.setLastSigningBlockHeight(Height(10));

		// Assert:
		EXPECT_EQ(Height(10), entry.lastSigningBlockHeight());
	}

	TEST(TEST_CLASS, CanSetEffectiveBalance) {
		// Arrange:
		auto entry = CommitteeEntry(Key(), Key(), Height(), Importance(5), false, 0.0, 0.0);

		// Sanity check:
		EXPECT_EQ(Importance(5), entry.effectiveBalance());

		// Act:
		entry.setEffectiveBalance(Importance(10));

		// Assert:
		EXPECT_EQ(Importance(10), entry.effectiveBalance());
	}

	TEST(TEST_CLASS, CanSetCanHarvest) {
		// Arrange:
		auto entry = CommitteeEntry(Key(), Key(), Height(), Importance(), false, 0.0, 0.0);

		// Sanity check:
		EXPECT_EQ(false, entry.canHarvest());

		// Act:
		entry.setCanHarvest(true);

		// Assert:
		EXPECT_EQ(true, entry.canHarvest());
	}

	TEST(TEST_CLASS, CanSetActivity) {
		// Arrange:
		auto entry = CommitteeEntry(Key(), Key(), Height(), Importance(), true, 5.0, 0.0);

		// Sanity check:
		EXPECT_EQ(5.0, entry.activity());

		// Act:
		entry.setActivity(10.0);

		// Assert:
		EXPECT_EQ(10.0, entry.activity());
	}

	TEST(TEST_CLASS, CanSetGreed) {
		// Arrange:
		auto entry = CommitteeEntry(Key(), Key(), Height(), Importance(), false, 5.0, 0.5);

		// Sanity check:
		EXPECT_EQ(0.5, entry.greed());

		// Act:
		entry.setGreed(1.0);

		// Assert:
		EXPECT_EQ(1.0, entry.greed());
	}
}}
