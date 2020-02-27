/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
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

	TEST(TEST_CLASS, CanAccessStart) {
		// Arrange:
		auto entry = SuperContractEntry(Key());

		// Sanity:
		ASSERT_EQ(Height(0), entry.start());

		// Act:
		entry.setStart(Height(10));

		// Assert:
		EXPECT_EQ(Height(10), entry.start());
	}

	TEST(TEST_CLASS, CanAccessEnd) {
		// Arrange:
		auto entry = SuperContractEntry(Key());

		// Sanity:
		ASSERT_EQ(Height(0), entry.end());

		// Act:
		entry.setEnd(Height(10));

		// Assert:
		EXPECT_EQ(Height(10), entry.end());
	}

	TEST(TEST_CLASS, CanAccessVmVersion) {
		// Arrange:
		auto vmVersion = VmVersion(10);
		auto entry = SuperContractEntry(Key());

		// Sanity:
		ASSERT_EQ(VmVersion(0), entry.vmVersion());

		// Act:
		entry.setVmVersion(vmVersion);

		// Assert:
		EXPECT_EQ(vmVersion, entry.vmVersion());
	}

	TEST(TEST_CLASS, CanAccessMainDriveKey) {
		// Arrange:
		auto mainDriveKey = test::GenerateRandomByteArray<Key>();
		auto entry = SuperContractEntry(Key());

		// Sanity:
		ASSERT_EQ(Key(), entry.mainDriveKey());

		// Act:
		entry.setMainDriveKey(mainDriveKey);

		// Assert:
		EXPECT_EQ(mainDriveKey, entry.mainDriveKey());
	}

	TEST(TEST_CLASS, CanAccessFileHash) {
		// Arrange:
		auto fileHash = test::GenerateRandomByteArray<Hash256>();
		auto entry = SuperContractEntry(Key());

		// Sanity:
		ASSERT_EQ(Hash256(), entry.fileHash());

		// Act:
		entry.setFileHash(fileHash);

		// Assert:
		EXPECT_EQ(fileHash, entry.fileHash());
	}
}}
