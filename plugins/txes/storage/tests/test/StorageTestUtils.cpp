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
                EXPECT_EQ(expectedActiveDataModification.ExpectedUploadSizeMegabytes, activeDataModification.ExpectedUploadSizeMegabytes);
				EXPECT_EQ(expectedActiveDataModification.ActualUploadSizeMegabytes, activeDataModification.ActualUploadSizeMegabytes);
				EXPECT_EQ(expectedActiveDataModification.FolderName, activeDataModification.FolderName);
				EXPECT_EQ(expectedActiveDataModification.ReadyForApproval, activeDataModification.ReadyForApproval);
            }
        }

        void AssertEqualCompletedDataModifications(const std::vector<state::CompletedDataModification>& expectedCompletedDataModifications, const std::vector<state::CompletedDataModification>& completedDataModifications) {
            ASSERT_EQ(expectedCompletedDataModifications.size(), completedDataModifications.size());
            for (auto i = 0u; i < completedDataModifications.size(); i++) {
                const auto &expectedCompletedDataModification = expectedCompletedDataModifications[i];
                const auto &completedDataModification = completedDataModifications[i];
                EXPECT_EQ(expectedCompletedDataModification.Id, completedDataModification.Id);
                EXPECT_EQ(expectedCompletedDataModification.Owner, completedDataModification.Owner);
                EXPECT_EQ(expectedCompletedDataModification.DownloadDataCdi, completedDataModification.DownloadDataCdi);
                EXPECT_EQ(expectedCompletedDataModification.ExpectedUploadSizeMegabytes, completedDataModification.ExpectedUploadSizeMegabytes);
				EXPECT_EQ(expectedCompletedDataModification.ActualUploadSizeMegabytes, completedDataModification.ActualUploadSizeMegabytes);
				EXPECT_EQ(expectedCompletedDataModification.FolderName, completedDataModification.FolderName);
                EXPECT_EQ(expectedCompletedDataModification.State, completedDataModification.State);
                EXPECT_EQ(expectedCompletedDataModification.ReadyForApproval, expectedCompletedDataModification.ReadyForApproval);
            }
        }

		void AssertEqualVerifications(const state::Verifications& expectedVerifications, const state::Verifications& verifications) {
			ASSERT_EQ(expectedVerifications.size(), verifications.size());
			for (auto i = 0u; i < verifications.size(); i++) {
				const auto &expectedVerification = expectedVerifications[i];
				const auto &verification = verifications[i];
				EXPECT_EQ(expectedVerification.VerificationTrigger, verification.VerificationTrigger);
				EXPECT_EQ(expectedVerification.Expiration, verification.Expiration);
				EXPECT_EQ(expectedVerification.Expired, verification.Expired);
				ASSERT_EQ(expectedVerification.Shards.size(), verification.Shards.size());
				for (auto i = 0u; i < expectedVerification.Shards.size(); ++i) {
					ASSERT_EQ(expectedVerification.Shards[i].size(), verification.Shards[i].size());
					for (auto k = 0u; k < expectedVerification.Shards[i].size(); ++k)
						EXPECT_EQ(expectedVerification.Shards[i][k], verification.Shards[i][k]);
				}
			}
		}

		void AssertEqualDriveInfos(const std::map<Key, state::DriveInfo>& expectedDriveInfos, const std::map<Key, state::DriveInfo>& driveInfos) {
			ASSERT_EQ(expectedDriveInfos.size(), driveInfos.size());
			for (const auto& pair : driveInfos) {
				const auto expIter = expectedDriveInfos.find(pair.first);
				ASSERT_NE(expIter, expectedDriveInfos.end());
				EXPECT_EQ(expIter->second.LastApprovedDataModificationId, pair.second.LastApprovedDataModificationId);
				EXPECT_EQ(expIter->second.DataModificationIdIsValid, pair.second.DataModificationIdIsValid);
				EXPECT_EQ(expIter->second.InitialDownloadWorkMegabytes, pair.second.InitialDownloadWorkMegabytes);
			}
		}

        void AssertEqualActiveDownloads(const std::vector<Hash256>& expectedActiveDownloads, const std::vector<Hash256>& activeDownloads) {
            ASSERT_EQ(expectedActiveDownloads.size(), activeDownloads.size());
            for (auto i = 0u; i < activeDownloads.size(); ++i) {
                EXPECT_EQ(expectedActiveDownloads[i], activeDownloads[i]);
            }
        }

        void AssertEqualCompletedDownloads(const std::vector<Hash256>& expectedCompletedDownloads, const std::vector<Hash256>& completedDownloads) {
            ASSERT_EQ(expectedCompletedDownloads.size(), completedDownloads.size());
            for (auto i = 0u; i < completedDownloads.size(); ++i) {
                EXPECT_EQ(expectedCompletedDownloads[i], completedDownloads[i]);
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
            uint16_t verificationsCount,
		    uint16_t activeDownloadsCount,
		    uint16_t completedDownloadsCount) {
        state::BcDriveEntry entry(key);
        entry.setOwner(owner);
        entry.setRootHash(rootHash);
        entry.setSize(size);
        entry.setReplicatorCount(replicatorCount);

        entry.activeDataModifications().reserve(activeDataModificationsCount);
        for (auto aDMC = 0u; aDMC < activeDataModificationsCount; ++aDMC){
			auto folderNameBytes = test::GenerateRandomVector(512);
			auto uploadSize = test::Random();
			bool readyForApproval = test::RandomByte();
			entry.activeDataModifications().emplace_back(state::ActiveDataModification(
                test::GenerateRandomByteArray<Hash256>(),   					/// Id of data modification.
				key,                                        					/// Public key of the drive owner.
                test::GenerateRandomByteArray<Hash256>(),   					/// CDI of download data.
				uploadSize,                             						/// ExpectedUpload size of data.
				uploadSize,														/// ActualUpload size of data.
				std::string(folderNameBytes.begin(), folderNameBytes.end()),	/// FolderName (for stream)
				readyForApproval												/// Flag whether modification can be approved
			));
        }

        entry.completedDataModifications().reserve(completedDataModificationsCount);
        for (auto cDMC = 0u; cDMC < completedDataModificationsCount; ++cDMC){
            entry.completedDataModifications().emplace_back(state::CompletedDataModification{
                entry.activeDataModifications()[cDMC], static_cast<state::DataModificationState>(test::RandomByte())
            });
        }

        entry.verifications().reserve(verificationsCount);
        for (auto i = 0u; i < verificationsCount; ++i) {
			entry.verifications().emplace_back(state::Verification{test::GenerateRandomByteArray<Hash256>(), Timestamp(test::Random()), bool(test::RandomByte()), {}});
			entry.verifications().back().Shards.emplace_back();
			auto& shard = entry.verifications().back().Shards.back();
			for (uint16_t k = 0u; k < replicatorCount; ++k)
				shard.emplace_back(test::GenerateRandomByteArray<Key>());
		}

        return entry;
    }

    void AssertEqualBcDriveData(const state::BcDriveEntry& expectedEntry, const state::BcDriveEntry& entry) {
        EXPECT_EQ(expectedEntry.key(), entry.key());
        EXPECT_EQ(expectedEntry.owner(), entry.owner());
        EXPECT_EQ(expectedEntry.rootHash(), entry.rootHash());
        EXPECT_EQ(expectedEntry.size(), entry.size());
		EXPECT_EQ(expectedEntry.usedSizeBytes(), entry.usedSizeBytes());
		EXPECT_EQ(expectedEntry.metaFilesSizeBytes(), entry.metaFilesSizeBytes());
        EXPECT_EQ(expectedEntry.replicatorCount(), entry.replicatorCount());
		EXPECT_EQ(expectedEntry.ownerCumulativeUploadSizeBytes(), entry.ownerCumulativeUploadSizeBytes());
		EXPECT_EQ(expectedEntry.confirmedUsedSizes(), entry.confirmedUsedSizes());
		EXPECT_EQ(expectedEntry.replicators(), entry.replicators());

        AssertEqualActiveDataModifications(expectedEntry.activeDataModifications(), entry.activeDataModifications());
        AssertEqualCompletedDataModifications(expectedEntry.completedDataModifications(), entry.completedDataModifications());
		AssertEqualVerifications(expectedEntry.verifications(), entry.verifications());
    }

    state::DownloadChannelEntry CreateDownloadChannelEntry(
            Hash256 id,
            Key consumer,
			Key drive,
			uint64_t downloadSize,
			uint16_t downloadApprovalCount,
			std::vector<Key> listOfPublicKeys,
			std::map<Key, Amount> cumulativePayments) {
        state::DownloadChannelEntry entry(id);
        entry.setConsumer(consumer);
		entry.setDrive(drive);
		entry.setDownloadSize(downloadSize);
		entry.setDownloadApprovalCount(downloadApprovalCount);
		entry.listOfPublicKeys() = listOfPublicKeys;
		entry.cumulativePayments() = cumulativePayments;

        return entry;
    }

	void AssertEqualListOfPublicKeys(const std::vector<Key>& expected, const std::vector<Key>& actual) {
		EXPECT_EQ(expected.size(), actual.size());
		for (int i = 0; i < expected.size(); i++) {
			EXPECT_EQ(expected[i], actual[i]);
		}
	}

	void AssertEqualCumulativePayments(const std::map<Key, Amount>& expected,
									   const std::map<Key, Amount>& actual) {
		EXPECT_EQ(expected.size(), actual.size());
		auto itExpected = expected.begin();
		auto itActual = actual.begin();
		for(; itExpected != expected.end(); itExpected++) {
			EXPECT_EQ(itExpected->first, itActual->first);
			EXPECT_EQ(itExpected->second, itActual->second);
		}
	}

    void AssertEqualDownloadChannelData(const state::DownloadChannelEntry& expectedEntry, const state::DownloadChannelEntry& entry) {
        EXPECT_EQ(expectedEntry.id(), entry.id());
        EXPECT_EQ(expectedEntry.consumer(), entry.consumer());
		EXPECT_EQ(expectedEntry.drive(), entry.drive());
		EXPECT_EQ(expectedEntry.downloadSize(), entry.downloadSize());
		EXPECT_EQ(expectedEntry.downloadApprovalCount(), entry.downloadApprovalCount());

		AssertEqualListOfPublicKeys(expectedEntry.listOfPublicKeys(), entry.listOfPublicKeys());
		AssertEqualCumulativePayments(expectedEntry.cumulativePayments(), entry.cumulativePayments());
    }

    state::ReplicatorEntry CreateReplicatorEntry(
            Key key,
            Amount capacity,
            uint16_t drivesCount,
			uint16_t downloadChannelCount) {
        state::ReplicatorEntry entry(key);
		entry.setCapacity(capacity);
        for (auto dC = 0u; dC < drivesCount; ++dC)
            entry.drives().emplace(test::GenerateRandomByteArray<Key>(), state::DriveInfo());
        for (auto i = 0u; i < downloadChannelCount; ++i)
            entry.downloadChannels().emplace_back(test::GenerateRandomByteArray<Hash256>());

        return entry;
    }

    void AssertEqualReplicatorData(const state::ReplicatorEntry& expectedEntry, const state::ReplicatorEntry& entry) {
        EXPECT_EQ(expectedEntry.key(), entry.key());
        EXPECT_EQ(expectedEntry.capacity(), entry.capacity());
		EXPECT_EQ(expectedEntry.blsKey(), entry.blsKey());

		AssertEqualDriveInfos(expectedEntry.drives(), entry.drives());
    }

	void AddReplicators(cache::CatapultCache& cache, std::vector<crypto::KeyPair>& replicatorKeyPairs, const uint8_t count, const Height height) {
		auto delta = cache.createDelta();
		auto& replicatorDelta = delta.sub<cache::ReplicatorCache>();
		const auto newSize = replicatorKeyPairs.size() + count;
		replicatorKeyPairs.reserve(newSize);
		for (auto i = replicatorKeyPairs.size(); i < newSize; ++i) {
			replicatorKeyPairs.emplace_back(crypto::KeyPair::FromPrivate(crypto::PrivateKey::Generate(test::RandomByte)));
			const auto& key = replicatorKeyPairs.at(i).publicKey();
			const auto replicatorEntry = test::CreateReplicatorEntry(key);
			replicatorDelta.insert(replicatorEntry);
		}
		cache.commit(height);
	}

	RawBuffer GenerateCommonDataBuffer(const size_t& size) {
		auto* const pCommonData = new uint8_t[size];
		MutableRawBuffer mutableBuffer(pCommonData, size);
		test::FillWithRandomData(mutableBuffer);
		return RawBuffer(mutableBuffer.pData, mutableBuffer.Size);
	}

	void PopulateReplicatorKeyPairs(std::vector<crypto::KeyPair>& replicatorKeyPairs, uint16_t replicatorCount) {
		replicatorKeyPairs.reserve(replicatorCount);
		for (auto i = replicatorKeyPairs.size(); i < replicatorCount; ++i)
			replicatorKeyPairs.emplace_back(crypto::KeyPair::FromPrivate(crypto::PrivateKey::Generate(test::RandomByte)));
	};

	void AddAccountState(
			cache::AccountStateCacheDelta& accountStateCache,
			const Key& publicKey,
			const Height& height,
			const std::vector<model::Mosaic>& mosaics){
		accountStateCache.addAccount(publicKey, height);
		auto accountStateIter = accountStateCache.find(publicKey);
		auto& accountState = accountStateIter.get();
		for (auto& mosaic : mosaics)
			accountState.Balances.credit(mosaic.MosaicId, mosaic.Amount);
	}
}}


