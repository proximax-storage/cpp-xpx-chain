/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
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
			EXPECT_NE(lockFundRecord.Get().find(MosaicId(272)), lockFundRecord.Get().end());

		lockFundRecord.Reactivate();

		// Sanity check:
		EXPECT_TRUE(lockFundRecord.Active());
		EXPECT_EQ(0, lockFundRecord.Size());
		EXPECT_NE(lockFundRecord.Get().find(MosaicId(272)), lockFundRecord.Get().end());

		lockFundRecord.Unset();

		// Sanity check:
		EXPECT_FALSE(lockFundRecord.Active());
		EXPECT_EQ(0, lockFundRecord.Size());
		EXPECT_EQ(lockFundRecord.Get().find(MosaicId(72)), lockFundRecord.Get().end());

	}

	// endregion
}}
