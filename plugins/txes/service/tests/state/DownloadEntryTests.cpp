/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/state/DownloadEntry.h"
#include "tests/TestHarness.h"

namespace catapult { namespace state {

#define TEST_CLASS DownloadEntryTests

	TEST(TEST_CLASS, CanCreateDefaultDownloadEntry) {
		// Act:
		DownloadEntry entry;

		// Assert:
		EXPECT_EQ(Hash256(), entry.OperationToken);
		EXPECT_EQ(Key(), entry.DriveKey);
		EXPECT_EQ(Key(), entry.FileRecipient);
		EXPECT_EQ(Height(), entry.Height);
		EXPECT_TRUE(entry.Files.empty());
	}

	TEST(TEST_CLASS, CanCreateDownloadEntryWithOperationToken) {
		// Act:
		auto operationToken = test::GenerateRandomByteArray<Hash256>();
		auto entry = DownloadEntry(operationToken);

		// Assert:
		EXPECT_EQ(operationToken, entry.OperationToken);
		EXPECT_EQ(Key(), entry.DriveKey);
		EXPECT_EQ(Key(), entry.FileRecipient);
		EXPECT_EQ(Height(), entry.Height);
		EXPECT_TRUE(entry.Files.empty());
	}

	TEST(TEST_CLASS, IsActiveReturnsTrueWhenNotExpiredAndTheareAreFiles) {
		// Act:
		DownloadEntry entry;
		entry.Height = Height(10);
		entry.Files.emplace(test::GenerateRandomByteArray<Hash256>(), test::Random());

		// Assert:
		EXPECT_TRUE(entry.isActive(Height(5)));
	}

	TEST(TEST_CLASS, IsActiveReturnsFalseWhenExpired) {
		// Act:
		DownloadEntry entry;
		entry.Height = Height(10);
		entry.Files.emplace(test::GenerateRandomByteArray<Hash256>(), test::Random());

		// Assert:
		EXPECT_FALSE(entry.isActive(Height(20)));
	}

	TEST(TEST_CLASS, IsActiveReturnsFalseWhenNoFiles) {
		// Act:
		DownloadEntry entry;
		entry.Height = Height(10);

		// Assert:
		EXPECT_FALSE(entry.isActive(Height(5)));
	}

	TEST(TEST_CLASS, IsActiveReturnsFalseWhenExpiredAndNoFiles) {
		// Act:
		DownloadEntry entry;
		entry.Height = Height(10);

		// Assert:
		EXPECT_FALSE(entry.isActive(Height(20)));
	}
}}