/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/state/ReplicatorEntry.h"
#include "tests/TestHarness.h"

namespace catapult { namespace state {

#define TEST_CLASS ReplicatorEntryTests

    TEST(TEST_CLASS, ReplicatorEntry) {
        // Act:
        auto key = test::GenerateRandomByteArray<Key>();
        auto entry = ReplicatorEntry(key);

        // Assert:
        EXPECT_EQ(key, entry.key());
    }

    TEST(TEST_CLASS, CanAccessDrives) {
        // Arrange:
        auto entry = ReplicatorEntry(Key());
        auto driveKey = test::GenerateRandomByteArray<Key>();
        DriveInfo driveInfo{ test::GenerateRandomByteArray<Hash256>(), 0 == test::RandomByte() % 2, test::Random() };

        // Sanity:
        ASSERT_TRUE(entry.drives().empty());

        // Act:
        entry.drives().emplace(driveKey, driveInfo);

        // Assert:
        ASSERT_EQ(1, entry.drives().size());
        EXPECT_EQ(driveInfo, entry.drives().at(driveKey));
    }

	TEST(TEST_CLASS, CanAccessDownloadChannels) {
		// Arrange:
		auto entry = ReplicatorEntry(Key());
		std::set<Hash256> downloadChannels = { test::GenerateRandomByteArray<Hash256>(), test::GenerateRandomByteArray<Hash256>() };

		// Sanity:
		ASSERT_TRUE(entry.downloadChannels().empty());

		// Act:
		entry.downloadChannels() = downloadChannels;

		// Assert:
		ASSERT_EQ(2, entry.downloadChannels().size());
		EXPECT_EQ(downloadChannels, entry.downloadChannels());
	}

}}