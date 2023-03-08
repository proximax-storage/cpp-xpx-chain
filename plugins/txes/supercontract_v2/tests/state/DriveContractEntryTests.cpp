/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/state/DriveContractEntry.h"
#include "tests/TestHarness.h"

namespace catapult { namespace state {

#define TEST_CLASS DriveContractEntryTests

	TEST(TEST_CLASS, CanCreateDriveContractEntry) {
		// Act:
		auto key = test::GenerateRandomByteArray<Key>();
		auto entry = DriveContractEntry(key);

		// Assert:
		EXPECT_EQ(key, entry.key());
	}

	TEST(TEST_CLASS, CanAccessContractKey) {
		// Arrange:
		auto contractKey = test::GenerateRandomByteArray<Key>();
		auto entry = DriveContractEntry(Key());

		// Sanity:
		ASSERT_EQ(Key(), entry.contractKey());

		// Act:
		entry.setContractKey(contractKey);

		// Assert:
		EXPECT_EQ(contractKey, entry.contractKey());
	}
}}