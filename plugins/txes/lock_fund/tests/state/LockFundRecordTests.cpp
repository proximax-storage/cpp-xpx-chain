/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/state/LockFundRecord.h"
#include "tests/TestHarness.h"

namespace catapult { namespace state {

#define TEST_CLASS LockFundRecordTests

	// region ctor

	TEST(TEST_CLASS, CanCreateLockFundRecordContainer) {
		// Arrange:
		auto lockFundRecord = state::LockFundRecord();
		// Assert:
		EXPECT_FALSE(lockFundRecord.Active());
		EXPECT_FALSE(lockFundRecord.Size());
	}

	// endregion

	TEST(TEST_CLASS, ContainerInactivatingAndReactivatingRecordsRetainsConsistency) {
		// Arrange:
		auto lockFundRecord = state::LockFundRecord();
		lockFundRecord.Set({{MosaicId(72), Amount(100)}});
		lockFundRecord.Inactivate();
		// Sanity check:
		EXPECT_FALSE(lockFundRecord.Active());
		EXPECT_EQ(1, lockFundRecord.Size());

		lockFundRecord.Set({{MosaicId(172), Amount(200)}});
		lockFundRecord.Inactivate();

		// Sanity check:
		EXPECT_FALSE(lockFundRecord.Active());
		EXPECT_EQ(2, lockFundRecord.Size());

		lockFundRecord.Set({{MosaicId(272), Amount(200)}});
		lockFundRecord.Inactivate();

		// Sanity check:
		EXPECT_FALSE(lockFundRecord.Active());
		EXPECT_EQ(3, lockFundRecord.Size());

		lockFundRecord.Reactivate();

		// Sanity check:
		EXPECT_TRUE(lockFundRecord.Active());
		EXPECT_EQ(2, lockFundRecord.Size());
		EXPECT_NE(lockFundRecord.Get().find(MosaicId(272)), lockFundRecord.Get().end());

		lockFundRecord.Reactivate();

		// Sanity check:
		EXPECT_TRUE(lockFundRecord.Active());
		EXPECT_EQ(1, lockFundRecord.Size());
		EXPECT_EQ(lockFundRecord.Get().find(MosaicId(272)), lockFundRecord.Get().end());

		lockFundRecord.Reactivate();

		// Sanity check:
		EXPECT_TRUE(lockFundRecord.Active());
		EXPECT_EQ(0, lockFundRecord.Size());
		EXPECT_EQ(lockFundRecord.Get().find(MosaicId(272)), lockFundRecord.Get().end());

		lockFundRecord.Unset();

		// Sanity check:
		EXPECT_FALSE(lockFundRecord.Active());
		EXPECT_EQ(0, lockFundRecord.Size());

	}

	// endregion
}}
