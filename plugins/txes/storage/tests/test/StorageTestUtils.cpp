/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "StorageTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace test {

    namespace {
        void AssertEqualActiveDataModifications(const std::vector<state::ActiveDataModification>& expectedActiveDataModifications, const std::vector<state::ActiveDataModification>& activeDataModifications) {
            ASSERT_EQ(expectedActiveDataModifications.size(), activeDataModifications.size());
            for (auto i = 0u; i < activeDataModifications.size(); i++) {
                const auto &expectedActiveDataModification = expectedActiveDataModifications[i];
                const auto &activeDataModification = activeDataModifications[i];
                EXPECT_EQ(expectedActiveDataModification.Id, activeDataModification.Id);
                EXPECT_EQ(expectedActiveDataModification.Owner, activeDataModification.Owner);
                EXPECT_EQ(expectedActiveDataModification.DownloadDataCdi, activeDataModification.DownloadDataCdi);
                EXPECT_EQ(expectedActiveDataModification.UploadSize, activeDataModification.UploadSize);
            }
        }

        void AssertEqualCompletedDataModifications(const std::vector<state::CompletedDataModification>& expectedCompletedDataModifications, const std::vector<state::CompletedDataModification>& completedDataModifications) {
            ASSERT_EQ(expectedCompletedDataModifications.size(), completedDataModifications.size());
            for (auto i = 0u; i < completedDataModifications.size(); i++) {
                const auto &expectedCompletedDataModification = expectedCompletedDataModifications[i];
                const auto &completedDataModification = completedDataModifications[i];
                EXPECT_EQ(expectedCompletedDataModifications[i].Id,completedDataModifications[i].Id);
                 EXPECT_EQ(expectedCompletedDataModifications[i].Owner,completedDataModifications[i].Owner);
                EXPECT_EQ(expectedCompletedDataModifications[i].DownloadDataCdi,completedDataModifications[i].DownloadDataCdi);
                EXPECT_EQ(expectedCompletedDataModifications[i].UploadSize,completedDataModifications[i].UploadSize);
                EXPECT_EQ(expectedCompletedDataModifications[i].State,completedDataModifications[i].State);
            }
        }
    }

    state::BcDriveEntry CreateBcDriveEntry(
            Key key,
            Key owner,
            Hash256 rootHash,
	    	uint64_t size,
	    	uint16_t replicatorCount,
	    	uint16_t activeDataModificationsCount,
		    uint16_t completedDataModificationsCount,
		    uint16_t activeDownloadsCount,
		    uint16_t completedDownloadsCount) {
        state::BcDriveEntry entry(key);
        entry.setOwner(owner);
        entry.setRootHash(rootHash);
        entry.setSize(size);
        entry.setReplicatorCount(replicatorCount);

        entry.activeDataModifications().reserve(activeDataModificationsCount);
        for (auto aDMC = 0u; aDMC < activeDataModificationsCount; ++aDMC){
            entry.activeDataModifications().emplace_back(state::ActiveDataModification{
                test::GenerateRandomByteArray<Hash256>(),   /// Id of data modification.
                key,                                        /// Public key of the drive owner.
                test::GenerateRandomByteArray<Hash256>(),   /// CDI of download data.
                test::Random()                              /// Upload size of data.
            });
        }

        entry.completedDataModifications().reserve(completedDataModificationsCount);
        for (auto cDMC = 0u; cDMC < completedDataModificationsCount; ++cDMC){
            entry.completedDataModifications().emplace_back(state::CompletedDataModification{
                state::CompletedDataModification(entry.activeDataModifications()[cDMC], static_cast<state::DataModificationState>(test::RandomByte())),
                static_cast<state::DataModificationState>(test::RandomByte())
            });
        }

        for (auto aDC = 0u; aDC < activeDownloadsCount; ++aDC) {
            entry.activeDownloads().emplace_back(test::GenerateRandomByteArray<Hash256>());
        }

        for (auto cDC = 0u; cDC < completedDownloadsCount; ++cDC) {
            entry.completedDownloads().emplace_back(test::GenerateRandomByteArray<Hash256>());
        }

        return entry;
    }

    void AssertEqualBcDriveData(const state::BcDriveEntry& expectedEntry, const state::BcDriveEntry& entry) {
        EXPECT_EQ(expectedEntry.key(), entry.key());
        EXPECT_EQ(expectedEntry.owner(), entry.owner());
        EXPECT_EQ(expectedEntry.rootHash(), entry.rootHash());
        EXPECT_EQ(expectedEntry.size(), entry.size());
        EXPECT_EQ(expectedEntry.replicatorCount(), entry.replicatorCount());

        AssertEqualActiveDataModifications(expectedEntry.activeDataModifications(), entry.activeDataModifications());
        AssertEqualCompletedDataModifications(expectedEntry.completedDataModifications(), entry.completedDataModifications());

        const auto& expectedActiveDownloads = expectedEntry.activeDownloads();
		const auto& activeDownloads = entry.activeDownloads();
        ASSERT_EQ(expectedActiveDownloads.size(), activeDownloads.size());
        for (auto i = 0u; i < activeDownloads.size(); ++i) {
            const auto& expectedActiveDownload = expectedActiveDownloads[i];
			const auto& activeDownload = activeDownloads[i];
            EXPECT_EQ(expectedActiveDownload.size(), activeDownload.size());
        }

        const auto& expectedCompletedDownloads = expectedEntry.completedDownloads();
		const auto& completedDownloads = entry.completedDownloads();
        ASSERT_EQ(expectedCompletedDownloads.size(), completedDownloads.size());
        for (auto i = 0u; i < completedDownloads.size(); ++i) {
            const auto& expectedCompletedDownload = expectedCompletedDownloads[i];
			const auto& completedDownload = completedDownloads[i];
            EXPECT_EQ(expectedCompletedDownload.size(), completedDownload.size());
        }
    }

    state::DownloadChannelEntry CreateDownloadChannelEntry(
            Hash256 id,
            Key consumer,
		    Key drive,
    		Amount transactionFee,
	    	Amount storageUnits) {
        state::DownloadChannelEntry entry(id);
        entry.setConsumer(consumer);
        entry.setDrive(drive);
        entry.setTransactionFee(transactionFee);
        entry.setStorageUnits(storageUnits);

        return entry;
    }

    void AssertEqualDownloadChannelData(const state::DownloadChannelEntry& expectedEntry, const state::DownloadChannelEntry& entry) {
        EXPECT_EQ(expectedEntry.id(), entry.id());
        EXPECT_EQ(expectedEntry.consumer(), entry.consumer());
        EXPECT_EQ(expectedEntry.drive(), entry.drive());
        EXPECT_EQ(expectedEntry.transactionFee(), entry.transactionFee());
        EXPECT_EQ(expectedEntry.storageUnits(), entry.storageUnits());
    }

    state::ReplicatorEntry CreateReplicatorEntry(
            Key key,
            Amount capacity,
            uint16_t drivesCount) {
        state::ReplicatorEntry entry(key);
        entry.setCapacity(capacity);
        for (auto dC = 0u; dC < drivesCount; ++dC)
            entry.drives().emplace_back(test::GenerateRandomByteArray<Key>());

        return entry;
    }

    void AssertEqualReplicatorData(const state::ReplicatorEntry& expectedEntry, const state::ReplicatorEntry& entry) {
        EXPECT_EQ(expectedEntry.key(), entry.key());
        EXPECT_EQ(expectedEntry.capacity(), entry.capacity());

        const auto& expectedDrives = expectedEntry.drives();
		const auto& drives = entry.drives();
        ASSERT_EQ(expectedDrives.size(), drives.size());
        for (auto i = 0u; i < drives.size(); ++i) {
            const auto& expectedDrive = expectedDrives[i];
			const auto& drive = drives[i];
            EXPECT_EQ(expectedDrive.size(), drive.size());
        }
    }

}}


