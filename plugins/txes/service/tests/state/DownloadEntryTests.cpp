/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/state/DownloadEntry.h"
#include "tests/TestHarness.h"

namespace catapult { namespace state {

#define TEST_CLASS DownloadEntryTests

	TEST(TEST_CLASS, CanCreateDownloadEntry) {
		// Act:
		auto driveKey = test::GenerateRandomByteArray<Key>();
		auto entry = DownloadEntry(driveKey);

		// Assert:
		EXPECT_EQ(driveKey, entry.driveKey());
	}

	TEST(TEST_CLASS, CanAccessFileRecipients) {
		// Arrange:
		auto fileRecipient = test::GenerateRandomByteArray<Key>();
		auto operationToken = test::GenerateRandomByteArray<Hash256>();
		std::set<Hash256> fileHashes{ test::GenerateRandomByteArray<Hash256>(), test::GenerateRandomByteArray<Hash256>() };
		auto entry = DownloadEntry(Key());

		// Sanity:
		ASSERT_TRUE(entry.fileRecipients().empty());

		// Act:
		for (const auto& fileHash : fileHashes) {
			entry.fileRecipients()[fileRecipient][operationToken].insert(fileHash);
		}

		// Assert:
		ASSERT_EQ(1, entry.fileRecipients().size());
		ASSERT_EQ(1, entry.fileRecipients().at(fileRecipient).size());
		ASSERT_EQ(2, entry.fileRecipients().at(fileRecipient).at(operationToken).size());
		for (const auto& fileHash : fileHashes) {
			EXPECT_EQ(1, entry.fileRecipients().at(fileRecipient).at(operationToken).count(fileHash));
		}
	}
}}