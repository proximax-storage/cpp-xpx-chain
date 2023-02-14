/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/state/SuperContractEntry.h"
#include "tests/TestHarness.h"

namespace catapult { namespace state {

#define TEST_CLASS SuperContractEntryTests

	TEST(TEST_CLASS, CanCreateSuperContractEntry) {
		// Act:
		auto key = test::GenerateRandomByteArray<Key>();
		auto entry = SuperContractEntry(key);

		// Assert:
		EXPECT_EQ(key, entry.key());
	}

	TEST(TEST_CLASS, CanAccessDriveKey) {
		// Arrange:
		auto driveKey = test::GenerateRandomByteArray<Key>();
		auto entry = SuperContractEntry(Key());

		// Sanity:
		ASSERT_EQ(Key(), entry.driveKey());

		// Act:
		entry.setDriveKey(driveKey);

		// Assert:
		EXPECT_EQ(driveKey, entry.driveKey());
	}

	TEST(TEST_CLASS, CanAccessAssignee) {
		// Arrange:
		auto assigneeKey = test::GenerateRandomByteArray<Key>();
		auto entry = SuperContractEntry(Key());

		// Sanity:
		ASSERT_EQ(Key(), entry.assignee());

		// Act:
		entry.setAssignee(assigneeKey);

		// Assert:
		EXPECT_EQ(assigneeKey, entry.assignee());
	}

	TEST(TEST_CLASS, CanAccess) {
		// Arrange:
		auto executionPaymentKey = test::GenerateRandomByteArray<Key>();
		auto entry = SuperContractEntry(Key());

		// Sanity:
		ASSERT_EQ(Key(), entry.executionPaymentKey());

		// Act:
		entry.setExecutionPaymentKey(executionPaymentKey);

		// Assert:
		EXPECT_EQ(executionPaymentKey, entry.executionPaymentKey());
	}
}}