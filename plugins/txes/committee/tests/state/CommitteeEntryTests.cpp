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

	TEST(TEST_CLASS, CanSetActivityObsolete) {
		// Arrange:
		auto entry = CommitteeEntry(Key(), Key(), Height(), Importance(), true, 5.0, 0.0);

		// Sanity check:
		EXPECT_EQ(5.0, entry.activityObsolete());

		// Act:
		entry.setActivityObsolete(10.0);

		// Assert:
		EXPECT_EQ(10.0, entry.activityObsolete());
	}

	TEST(TEST_CLASS, CanSetGreedObsolete) {
		// Arrange:
		auto entry = CommitteeEntry(Key(), Key(), Height(), Importance(), false, 5.0, 0.5);

		// Sanity check:
		EXPECT_EQ(0.5, entry.greedObsolete());

		// Act:
		entry.setGreedObsolete(1.0);

		// Assert:
		EXPECT_EQ(1.0, entry.greedObsolete());
	}

	TEST(TEST_CLASS, CanSetExpirationTime) {
		// Arrange:
		auto entry = CommitteeEntry(Key(), Key(), Height(), Importance(), false, 5.0, 0.5);

		// Sanity check:
		EXPECT_EQ(Timestamp(0), entry.expirationTime());

		// Act:
		entry.setExpirationTime(Timestamp(5));

		// Assert:
		EXPECT_EQ(Timestamp(5), entry.expirationTime());
	}

	TEST(TEST_CLASS, CanSetActivity) {
		// Arrange:
		auto entry = CommitteeEntry(Key(), Key(), Height(), Importance(), false, 5.0, 0.5);

		// Sanity check:
		EXPECT_EQ(0, entry.activity());

		// Act:
		entry.setActivity(15);

		// Assert:
		EXPECT_EQ(15, entry.activity());
	}

	TEST(TEST_CLASS, CanSetFeeInterest) {
		// Arrange:
		auto entry = CommitteeEntry(Key(), Key(), Height(), Importance(), false, 5.0, 0.5);

		// Sanity check:
		EXPECT_EQ(0, entry.feeInterest());

		// Act:
		entry.setFeeInterest(25);

		// Assert:
		EXPECT_EQ(25, entry.feeInterest());
	}

	TEST(TEST_CLASS, CanSetFeeInterestDenominator) {
		// Arrange:
		auto entry = CommitteeEntry(Key(), Key(), Height(), Importance(), false, 5.0, 0.5);

		// Sanity check:
		EXPECT_EQ(0, entry.feeInterestDenominator());

		// Act:
		entry.setFeeInterestDenominator(35);

		// Assert:
		EXPECT_EQ(35, entry.feeInterestDenominator());
	}

	TEST(TEST_CLASS, CanSetBootKey) {
		// Arrange:
		auto entry = CommitteeEntry(Key(), Key(), Height(), Importance(), false, 5.0, 0.5);
		auto bootKey = test::GenerateRandomByteArray<Key>();

		// Sanity check:
		EXPECT_EQ(Key(), entry.bootKey());

		// Act:
		entry.setBootKey(bootKey);

		// Assert:
		EXPECT_EQ(bootKey, entry.bootKey());
	}

	TEST(TEST_CLASS, CanSetBlockchainVersion) {
		// Arrange:
		auto entry = CommitteeEntry(Key(), Key(), Height(), Importance(), false, 5.0, 0.5);
		auto blockchainVersion = BlockchainVersion(test::Random());

		// Sanity check:
		EXPECT_EQ(BlockchainVersion(), entry.blockchainVersion());

		// Act:
		entry.setBlockchainVersion(blockchainVersion);

		// Assert:
		EXPECT_EQ(blockchainVersion, entry.blockchainVersion());
	}
}}
