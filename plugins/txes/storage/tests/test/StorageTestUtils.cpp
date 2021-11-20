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
                EXPECT_EQ(expectedCompletedDataModification.Id, completedDataModification.Id);
                EXPECT_EQ(expectedCompletedDataModification.Owner, completedDataModification.Owner);
                EXPECT_EQ(expectedCompletedDataModification.DownloadDataCdi, completedDataModification.DownloadDataCdi);
                EXPECT_EQ(expectedCompletedDataModification.UploadSize, completedDataModification.UploadSize);
                EXPECT_EQ(expectedCompletedDataModification.State, completedDataModification.State);
            }
        }

		void AssertEqualReplicatorInfos(const std::map<Key, state::ReplicatorInfo>& expectedReplicatorInfos, const std::map<Key, state::ReplicatorInfo>& replicatorInfos) {
			ASSERT_EQ(expectedReplicatorInfos.size(), replicatorInfos.size());
			for (auto iter = replicatorInfos.begin(); iter != replicatorInfos.end(); ++iter) {
				const auto expIter = expectedReplicatorInfos.find(iter->first);
				ASSERT_NE(expIter, expectedReplicatorInfos.end());
				EXPECT_EQ(expIter->second.UsedSize, iter->second.UsedSize);
				EXPECT_EQ(expIter->second.CumulativeUploadSize, iter->second.CumulativeUploadSize);
			}
		}

		void AssertEqualDriveInfos(const std::map<Key, state::DriveInfo>& expectedDriveInfos, const std::map<Key, state::DriveInfo>& driveInfos) {
			ASSERT_EQ(expectedDriveInfos.size(), driveInfos.size());
			for (auto iter = driveInfos.begin(); iter != driveInfos.end(); ++iter) {
				const auto expIter = expectedDriveInfos.find(iter->first);
				ASSERT_NE(expIter, expectedDriveInfos.end());
				EXPECT_EQ(expIter->second.LastApprovedDataModificationId, iter->second.LastApprovedDataModificationId);
				EXPECT_EQ(expIter->second.DataModificationIdIsValid, iter->second.DataModificationIdIsValid);
				EXPECT_EQ(expIter->second.InitialDownloadWork, iter->second.InitialDownloadWork);
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
                entry.activeDataModifications()[cDMC], static_cast<state::DataModificationState>(test::RandomByte())
            });
        }

        return entry;
    }

    void AssertEqualBcDriveData(const state::BcDriveEntry& expectedEntry, const state::BcDriveEntry& entry) {
        EXPECT_EQ(expectedEntry.key(), entry.key());
        EXPECT_EQ(expectedEntry.owner(), entry.owner());
        EXPECT_EQ(expectedEntry.rootHash(), entry.rootHash());
        EXPECT_EQ(expectedEntry.size(), entry.size());
		EXPECT_EQ(expectedEntry.usedSize(), entry.usedSize());
		EXPECT_EQ(expectedEntry.metaFilesSize(), entry.metaFilesSize());
        EXPECT_EQ(expectedEntry.replicatorCount(), entry.replicatorCount());
		EXPECT_EQ(expectedEntry.ownerCumulativeUploadSize(), entry.ownerCumulativeUploadSize());
		EXPECT_EQ(expectedEntry.replicators(), entry.replicators());

        AssertEqualActiveDataModifications(expectedEntry.activeDataModifications(), entry.activeDataModifications());
        AssertEqualCompletedDataModifications(expectedEntry.completedDataModifications(), entry.completedDataModifications());
		AssertEqualReplicatorInfos(expectedEntry.replicatorInfos(), entry.replicatorInfos());
    }

    state::DownloadChannelEntry CreateDownloadChannelEntry(
            Hash256 id,
            Key consumer,
			uint16_t downloadApprovalCount) {
        state::DownloadChannelEntry entry(id);
        entry.setConsumer(consumer);
		entry.setDownloadApprovalCount(downloadApprovalCount);

        return entry;
    }

    void AssertEqualDownloadChannelData(const state::DownloadChannelEntry& expectedEntry, const state::DownloadChannelEntry& entry) {
        EXPECT_EQ(expectedEntry.id(), entry.id());
        EXPECT_EQ(expectedEntry.consumer(), entry.consumer());
    }

    state::ReplicatorEntry CreateReplicatorEntry(
            Key key,
			BLSPublicKey blsKey,
            Amount capacity,
            uint16_t drivesCount) {
        state::ReplicatorEntry entry(key);
        entry.setBlsKey(blsKey);
		entry.setCapacity(capacity);
        for (auto dC = 0u; dC < drivesCount; ++dC)
            entry.drives().emplace(test::GenerateRandomByteArray<Key>(), state::DriveInfo());

        return entry;
    }

    void AssertEqualReplicatorData(const state::ReplicatorEntry& expectedEntry, const state::ReplicatorEntry& entry) {
        EXPECT_EQ(expectedEntry.key(), entry.key());
        EXPECT_EQ(expectedEntry.capacity(), entry.capacity());
		EXPECT_EQ(expectedEntry.blsKey(), entry.blsKey());

		AssertEqualDriveInfos(expectedEntry.drives(), entry.drives());
    }

	state::BlsKeysEntry CreateBlsKeysEntry(
			const BLSPublicKey& blsKey,
			const Key& key) {
		auto entry = state::BlsKeysEntry(blsKey);
		entry.setKey(key);
		return entry;
	};

	void AddReplicators(cache::CatapultCache& cache, std::vector<std::pair<Key, crypto::BLSKeyPair>>& replicatorKeyPairs, const uint8_t count, const Height height) {
		auto delta = cache.createDelta();
		auto& replicatorDelta = delta.sub<cache::ReplicatorCache>();
		auto& blsKeysDelta = delta.sub<cache::BlsKeysCache>();
		const auto newSize = replicatorKeyPairs.size() + count;
		replicatorKeyPairs.reserve(newSize);
		for (auto i = replicatorKeyPairs.size(); i < newSize; ++i) {
			replicatorKeyPairs.emplace_back(
					test::GenerateRandomByteArray<Key>(),
					crypto::BLSKeyPair::FromPrivate(crypto::BLSPrivateKey::Generate(test::RandomByte)));
			const auto& key = replicatorKeyPairs.at(i).first;
			const auto& blsPublicKey = replicatorKeyPairs.at(i).second.publicKey();
			const auto replicatorEntry = test::CreateReplicatorEntry(key, blsPublicKey);
			const auto blsKeyEntry = test::CreateBlsKeysEntry(blsPublicKey, key);
			replicatorDelta.insert(replicatorEntry);
			blsKeysDelta.insert(blsKeyEntry);
		}
		cache.commit(height);
	}

	RawBuffer GenerateCommonDataBuffer(const size_t& size) {
		auto* const pCommonData = new uint8_t[size];
		MutableRawBuffer mutableBuffer(pCommonData, size);
		test::FillWithRandomData(mutableBuffer);
		return RawBuffer(mutableBuffer.pData, mutableBuffer.Size);
	}

	void PopulateReplicatorKeyPairs(std::vector<std::pair<Key, crypto::BLSKeyPair>>& replicatorKeyPairs, uint16_t replicatorCount) {
		replicatorKeyPairs.reserve(replicatorCount);
		for (auto i = replicatorKeyPairs.size(); i < replicatorCount; ++i)
			replicatorKeyPairs.emplace_back(
					test::GenerateRandomByteArray<Key>(),
					crypto::BLSKeyPair::FromPrivate(crypto::BLSPrivateKey::Generate(test::RandomByte)));
	};
}}


