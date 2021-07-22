/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/state/BcDriveEntry.h"
#include "tests/TestHarness.h"

namespace catapult { namespace state {

#define TEST_CLASS BcDriveEntryTests

    TEST(TEST_CLASS, CanCreateBcDriveEntry) {
        // Act:
        auto key = test::GenerateRandomByteArray<Key>();
        auto entry = BcDriveEntry(key);

        // Assert:
        EXPECT_EQ(key, entry.key());
    }
    
    TEST(TEST_CLASS, CanAccessOwner) {
        // Arrange:
        auto owner = test::GenerateRandomByteArray<Key>();
        auto entry = BcDriveEntry(Key());

        // Sanity:
        ASSERT_EQ(Key(), entry.owner());

        // Act:
        entry.setOwner(owner);

        // Assert:
        EXPECT_EQ(owner, entry.owner());
    }

    TEST(TEST_CLASS, CanAccessRootHash) {
        // Arrange:
        auto rootHash = test::GenerateRandomByteArray<Hash256>();
        auto entry = BcDriveEntry(Key());

        // Sanity:
        ASSERT_EQ(Hash256(), entry.rootHash());

        // Act:
        entry.setRootHash(rootHash);

        // Assert:
        EXPECT_EQ(rootHash, entry.rootHash());
    }

    TEST(TEST_CLASS, CanAccessSize) {
        // Arrange:
        uint64_t size = 50u;
        auto entry = BcDriveEntry(Key());

        // Sanity:
        ASSERT_EQ(0u, entry.size());

        // Act:
        entry.setSize(size);

        // Assert:
        EXPECT_EQ(size, entry.size());
    }

    TEST(TEST_CLASS, CanAccessReplicatorCount) {
        // Arrange:
        uint16_t replicatorCount = 5u;
        auto entry = BcDriveEntry(Key());

        // Sanity:
        ASSERT_EQ(0u, entry.replicatorCount());

        // Act:
        entry.setReplicatorCount(replicatorCount);

        // Assert:
        EXPECT_EQ(replicatorCount, entry.replicatorCount());
    }

    TEST(TEST_CLASS, CanAccessActiveDataModification) {
        // Arrange:
        ActiveDataModification activeDataModification {
            test::GenerateRandomByteArray<Hash256>(),
            test::GenerateRandomByteArray<Key>(),
            test::GenerateRandomByteArray<Hash256>(),
            test::Random()
        };
        auto entry = BcDriveEntry(Key());

        // Sanity:
        ASSERT_TRUE(entry.activeDataModifications().empty());

        // Act:
        entry.activeDataModifications().emplace_back(activeDataModification);

        // Assert:
        ASSERT_EQ(1, entry.activeDataModifications().size());
        EXPECT_EQ(activeDataModification.Id, entry.activeDataModifications().back().Id);
        EXPECT_EQ(activeDataModification.Owner, entry.activeDataModifications().back().Owner);
        EXPECT_EQ(activeDataModification.DownloadDataCdi, entry.activeDataModifications().back().DownloadDataCdi);
        EXPECT_EQ(activeDataModification.UploadSize, entry.activeDataModifications().back().UploadSize);
    }

    TEST(TEST_CLASS, CanAccessCompletedDataModification) {
        // Arrange:
        ActiveDataModification activeDataModification {
            test::GenerateRandomByteArray<Hash256>(),
            test::GenerateRandomByteArray<Key>(),
            test::GenerateRandomByteArray<Hash256>(),
            test::Random()
        };
        CompletedDataModification completedDataModification {
            activeDataModification, DataModificationState::Succeeded
        };
        auto entry = BcDriveEntry(Key());

        // Sanity:
        ASSERT_TRUE(entry.completedDataModifications().empty());

        // Act:
        entry.completedDataModifications().emplace_back(completedDataModification);

        // Assert:
        ASSERT_EQ(1, entry.completedDataModifications().size());
        EXPECT_EQ(completedDataModification.Id, entry.completedDataModifications().back().Id);
        EXPECT_EQ(completedDataModification.Owner, entry.completedDataModifications().back().Owner);
        EXPECT_EQ(completedDataModification.DownloadDataCdi, entry.completedDataModifications().back().DownloadDataCdi);
        EXPECT_EQ(completedDataModification.UploadSize, entry.completedDataModifications().back().UploadSize);
        EXPECT_EQ(completedDataModification.State, entry.completedDataModifications().back().State);
    }

    TEST(TEST_CLASS, CanAccessActiveDownloads) {
        // Arrange:
        Hash256 activeDownloadId = test::GenerateRandomByteArray<Hash256>();
        auto entry = BcDriveEntry(Key());

        // Sanity:
        ASSERT_TRUE(entry.activeDownloads().empty());

        // Act:
        entry.activeDownloads().emplace_back(activeDownloadId);

        // Assert:
        ASSERT_EQ(1, entry.activeDownloads().size());
        EXPECT_EQ(activeDownloadId, entry.activeDownloads().back());
    }

    TEST(TEST_CLASS, CanAccessCompletedDownloads) {
        // Arrange:
        Hash256 completedDownloadId = test::GenerateRandomByteArray<Hash256>();
        auto entry = BcDriveEntry(Key());

        // Sanity:
        ASSERT_TRUE(entry.completedDownloads().empty());

        // Act:
        entry.completedDownloads().emplace_back(completedDownloadId);

        // Assert:
        ASSERT_EQ(1, entry.completedDownloads().size());
        EXPECT_EQ(completedDownloadId, entry.completedDownloads().back());
    }

    TEST(TEST_CLASS, CanAccessReplicators) {
        // Arrange:
        utils::KeySet replicators = { test::GenerateRandomByteArray<Key>(), test::GenerateRandomByteArray<Key>() };
        auto entry = BcDriveEntry(Key());

        // Sanity:
        ASSERT_TRUE(entry.replicators().empty());

        // Act:
        entry.replicators() = replicators;

        // Assert:
        ASSERT_EQ(2, entry.replicators().size());
        EXPECT_EQ(replicators, entry.replicators());
    }
    
}}