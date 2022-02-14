/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "tests/test/core/SerializerTestUtils.h"
#include "tests/test/StorageTestUtils.h"

namespace catapult { namespace state {

#define TEST_CLASS BcDriveEntrySerializerTests

	namespace {
		constexpr auto Replicator_Count = 10;
        constexpr auto Active_Data_Modifications_Count = 5;
        constexpr auto Completed_Data_Modifications_Count = 5;
        constexpr auto Verifications_Count = 1;

        constexpr auto Entry_Size =
            sizeof(VersionType) + // version
            Key_Size + // drive key
            Key_Size + // owner
            Hash256_Size + // root hash
            sizeof(uint64_t) + // size
            sizeof(uint64_t) + // used size
            sizeof(uint64_t) + // meta files size
            sizeof(uint16_t) + // replicator count
            sizeof(uint16_t) + // active data modifications count
            Active_Data_Modifications_Count * (Hash256_Size + Key_Size + Hash256_Size + sizeof(uint64_t)) + // active data modifications
            sizeof(uint16_t) + // completed data modifications count
            Completed_Data_Modifications_Count * (Hash256_Size + Key_Size + Hash256_Size + sizeof(uint64_t) + sizeof(uint8_t)) + // completed data modifications
            sizeof(uint16_t) + // verifications count
            Verifications_Count * (Hash256_Size + sizeof(uint16_t) + Replicator_Count * sizeof(uint16_t)); // verification data

        class TestContext {
        public:
            explicit TestContext()
                    : m_stream(m_buffer)
            {}

        public:
            auto& buffer() {
                return m_buffer;
            }

            auto& outputStream() {
                return m_stream;
            }

        private:
            std::vector<uint8_t> m_buffer;
            mocks::MockMemoryStream m_stream;
        };

        auto CreateBcDriveEntry() {
			auto key = test::GenerateRandomByteArray<Key>();
            return test::CreateBcDriveEntry(
                test::GenerateRandomByteArray<Key>(),
                test::GenerateRandomByteArray<Key>(),
                test::GenerateRandomByteArray<Hash256>(),
                test::Random(),
				Replicator_Count,
                Active_Data_Modifications_Count,
                Completed_Data_Modifications_Count,
				Verifications_Count,
				test::Random16(),
				test::Random16());
        }

		void AssertActiveDataModification(const ActiveDataModification& active, const uint8_t*& pData) {
			EXPECT_EQ_MEMORY(active.Id.data(), pData, Hash256_Size);
			pData += Hash256_Size;
			EXPECT_EQ_MEMORY(active.Owner.data(), pData, Key_Size);
			pData += Key_Size;
			EXPECT_EQ_MEMORY(active.DownloadDataCdi.data(), pData, Hash256_Size);
			pData += Hash256_Size;
			EXPECT_EQ(active.ExpectedUploadSizeMegabytes, *reinterpret_cast<const uint64_t*>(pData));
			pData += sizeof(uint64_t);
			EXPECT_EQ(active.ActualUploadSizeMegabytes, *reinterpret_cast<const uint64_t*>(pData));
			pData += sizeof(uint64_t);
			auto folderNameSize = active.FolderName.size();
			EXPECT_EQ(folderNameSize, *reinterpret_cast<const uint16_t*>(pData));
			pData += sizeof(uint16_t);
			EXPECT_EQ(active.ReadyForApproval, *reinterpret_cast<const uint8_t*>(pData));
			pData += sizeof(uint8_t);
			EXPECT_EQ_MEMORY(active.FolderName.c_str(), pData, folderNameSize);
			pData += folderNameSize;
		}

        void AssertActiveDataModifications(const ActiveDataModifications& activeDataModifications, const uint8_t*& pData) {
            EXPECT_EQ(activeDataModifications.size(), *reinterpret_cast<const uint16_t*>(pData));
            pData += sizeof(uint16_t);
            for (const auto& active : activeDataModifications) {
				AssertActiveDataModification(active, pData);
            }
        }

       	void AssertCompletedDataModifications(const CompletedDataModifications& completedDataModifications, const uint8_t*& pData) {
			EXPECT_EQ(completedDataModifications.size(), *reinterpret_cast<const uint16_t*>(pData));
            pData += sizeof(uint16_t);
            for (const auto& completed : completedDataModifications) {
				AssertActiveDataModification(completed, pData);
                EXPECT_EQ(completed.State, static_cast<DataModificationState>(*pData));
                pData++;
            }
		}

		void AssertConfirmedUsedSizes(const SizeMap& confirmedUsedSizes, const uint8_t*& pData) {
			EXPECT_EQ(confirmedUsedSizes.size(), *reinterpret_cast<const uint16_t*>(pData));
			pData += sizeof(uint16_t);
			for (const auto& pair : confirmedUsedSizes) {
				EXPECT_EQ_MEMORY(pair.first.data(), pData, Key_Size);
				pData += Key_Size;
				EXPECT_EQ(pair.second, *reinterpret_cast<const uint64_t*>(pData));
				pData += sizeof(uint64_t);
			}
		}

		void AssertReplicators(const utils::SortedKeySet& replicators, const uint8_t*& pData) {
			EXPECT_EQ(replicators.size(), *reinterpret_cast<const uint16_t*>(pData));
			pData += sizeof(uint16_t);
			for (const auto& replicator : replicators) {
				EXPECT_EQ_MEMORY(replicator.data(), pData, Key_Size);
				pData += Key_Size;
			}
		}

        void AssertVerifications(const Verifications& verifications, const uint8_t*& pData) {
            EXPECT_EQ(verifications.size(), *reinterpret_cast<const uint16_t*>(pData));
            pData += sizeof(uint16_t);
            for (const auto& verification : verifications) {
                EXPECT_EQ_MEMORY(verification.VerificationTrigger.data(), pData, Hash256_Size);
                pData += Hash256_Size;
                EXPECT_EQ(verification.Shards.size(), *reinterpret_cast<const uint16_t*>(pData));
                pData += sizeof(uint16_t);
				for (const auto& shard : verification.Shards) {
					EXPECT_EQ(shard.size(), *reinterpret_cast<const uint8_t*>(pData));
					pData += sizeof(uint8_t);
					for (const auto& key : shard) {
						EXPECT_EQ_MEMORY(key.data(), pData, Key_Size);
						pData += Key_Size;
					}
				}
            }
        }

        void AssertEntryBuffer(const state::BcDriveEntry& entry, const uint8_t* pData, size_t expectedSize, VersionType version) {
            const auto* pExpectedEnd = pData + expectedSize;
            EXPECT_EQ(version, *reinterpret_cast<const VersionType*>(pData));
			pData += sizeof(VersionType);
            EXPECT_EQ_MEMORY(entry.key().data(), pData, Key_Size);
            pData += Key_Size;
            EXPECT_EQ_MEMORY(entry.owner().data(), pData, Key_Size);
			pData += Key_Size;
            EXPECT_EQ_MEMORY(entry.rootHash().data(), pData, Hash256_Size);
			pData += Hash256_Size;
            EXPECT_EQ(entry.size(), *reinterpret_cast<const uint64_t*>(pData));
            pData += sizeof(uint64_t);
			EXPECT_EQ(entry.usedSizeBytes(), *reinterpret_cast<const uint64_t*>(pData));
			pData += sizeof(uint64_t);
			EXPECT_EQ(entry.metaFilesSizeBytes(), *reinterpret_cast<const uint64_t*>(pData));
			pData += sizeof(uint64_t);
            EXPECT_EQ(entry.replicatorCount(), *reinterpret_cast<const uint16_t*>(pData));
            pData += sizeof(uint16_t);
			EXPECT_EQ(entry.ownerCumulativeUploadSizeBytes(), *reinterpret_cast<const uint64_t*>(pData));
			pData += sizeof(uint64_t);

            AssertActiveDataModifications(entry.activeDataModifications(), pData);
            AssertCompletedDataModifications(entry.completedDataModifications(), pData);
			AssertConfirmedUsedSizes(entry.confirmedUsedSizes(), pData);
			AssertReplicators(entry.replicators(), pData);
			AssertReplicators(entry.offboardingReplicators(), pData);
            AssertVerifications(entry.verifications(), pData);

            EXPECT_EQ(pExpectedEnd, pData);
        }

		void AssertCanSaveSingleEntry(VersionType version) {
			// Arrange:
			TestContext context;
			auto entry = CreateBcDriveEntry();

			// Act:
			BcDriveEntrySerializer::Save(entry, context.outputStream());

			// Assert:
			AssertEntryBuffer(entry, context.buffer().data(), context.buffer().size(), version);
		}

		void AssertCanSaveMultipleEntries(VersionType version) {
			// Arrange:
			TestContext context;
			auto entry1 = CreateBcDriveEntry();
			auto entry2 = CreateBcDriveEntry();

			// Act:
			BcDriveEntrySerializer::Save(entry1, context.outputStream());
			auto entryBufferSize1 = context.buffer().size();
			BcDriveEntrySerializer::Save(entry2, context.outputStream());
			auto entryBufferSize2 = context.buffer().size() - entryBufferSize1;

			// Assert:
			const auto* pBuffer1 = context.buffer().data();
			const auto* pBuffer2 = pBuffer1 + entryBufferSize1;
			AssertEntryBuffer(entry1, pBuffer1, entryBufferSize1, version);
			AssertEntryBuffer(entry2, pBuffer2, entryBufferSize2, version);
		}
    }

    // region Save

    TEST(TEST_CLASS, CanSaveSingleEntry_v1) {
        AssertCanSaveSingleEntry(1);
    }

    TEST(TEST_CLASS, CanSaveMultipleEntries_v1) {
        AssertCanSaveMultipleEntries(1);
    }

    // endregion

    // region Load

    namespace {

		void CopyToVector(std::vector<uint8_t>& data, const uint8_t * p, size_t bytes) {
			data.insert(data.end(), p, p + bytes);
		}

		void SaveActiveDataModification(const ActiveDataModification& modification, std::vector<uint8_t>& data) {
			CopyToVector(data, modification.Id.data(), Hash256_Size);
			CopyToVector(data, modification.Owner.data(), Key_Size);
			CopyToVector(data, modification.DownloadDataCdi.data(), Hash256_Size);
			CopyToVector(data, (const uint8_t *) &modification.ExpectedUploadSizeMegabytes, sizeof(uint64_t));
			CopyToVector(data, (const uint8_t *) &modification.ActualUploadSizeMegabytes, sizeof(uint64_t));
			auto folderNameSize = (uint16_t) modification.FolderName.size();
			CopyToVector(data, (const uint8_t *) &folderNameSize, sizeof(folderNameSize));
			CopyToVector(data, (const uint8_t *) &modification.ReadyForApproval, sizeof(bool));
			CopyToVector(data, (const uint8_t *) modification.FolderName.c_str(), folderNameSize);
		}

        void SaveActiveDataModifications(const ActiveDataModifications& activeDataModifications, std::vector<uint8_t>& data) {
            uint16_t activeDataModificationsCount = utils::checked_cast<size_t, uint16_t>(activeDataModifications.size());
            CopyToVector(data, (const uint8_t *) &activeDataModificationsCount, sizeof(uint16_t));
			for (const auto& modification : activeDataModifications) {
				SaveActiveDataModification(modification, data);
			}
		}

        void SaveCompletedDataModifications(const CompletedDataModifications& completedDataModifications, std::vector<uint8_t>& data) {
		    uint16_t completedDataModificationsCount = utils::checked_cast<size_t, uint16_t>(completedDataModifications.size());
			CopyToVector(data, (const uint8_t *) &completedDataModificationsCount, sizeof(uint16_t));
            for (const auto& modification : completedDataModifications) {
				SaveActiveDataModification(modification, data);
                data.push_back(utils::to_underlying_type(modification.State));
            }
		}

		void SaveConfirmedUsedSizes(const SizeMap& confirmedUsedSizes, std::vector<uint8_t>& data) {
			uint16_t pairsCount = utils::checked_cast<size_t, uint16_t>(confirmedUsedSizes.size());
			CopyToVector(data, (const uint8_t *) &pairsCount, sizeof(uint16_t));
			for (const auto& pair : confirmedUsedSizes) {
				CopyToVector(data, pair.first.data(), Key_Size);
				CopyToVector(data, (const uint8_t *) &pair.second, sizeof(uint64_t));
			}
		}

		void SaveReplicators(const utils::SortedKeySet& replicators, std::vector<uint8_t>& data) {
			uint16_t replicatorsCount = utils::checked_cast<size_t, uint16_t>(replicators.size());
			CopyToVector(data, (const uint8_t *) &replicatorsCount, sizeof(uint16_t));
			for (const auto& replicator : replicators)
				CopyToVector(data, replicator.data(), Key_Size);
		}

        void SaveVerifications(const Verifications& verifications, std::vector<uint8_t>& data) {
            uint16_t verificationsCount = utils::checked_cast<size_t, uint16_t>(verifications.size());
			CopyToVector(data, (const uint8_t *) &verificationsCount, sizeof(uint16_t));
            for (const auto& verification : verifications) {
				CopyToVector(data, verification.VerificationTrigger.data(), Hash256_Size);
				uint16_t shardCount = utils::checked_cast<size_t, uint16_t>(verification.Shards.size());
				CopyToVector(data, (const uint8_t *) &shardCount, sizeof(uint16_t));
                for (const auto& shard : verification.Shards) {
					uint8_t shardSize = utils::checked_cast<size_t, uint8_t>(shard.size());
					CopyToVector(data, (const uint8_t *) &shardSize, sizeof(uint8_t));
					for (const auto& key : shard)
						CopyToVector(data, key.data(), Key_Size);
				}
            }
        }

        std::vector<uint8_t> CreateEntryBuffer(const state::BcDriveEntry& entry, VersionType version) {
            std::vector<uint8_t> data;
			CopyToVector(data, (const uint8_t*)&version, sizeof(version));
			CopyToVector(data, entry.key().data(), Key_Size);
			CopyToVector(data, entry.owner().data(), Key_Size);
			CopyToVector(data, entry.rootHash().data(), Hash256_Size);
			CopyToVector(data, (const uint8_t*) &entry.size(), sizeof(uint64_t));
			CopyToVector(data, (const uint8_t*) &entry.usedSizeBytes(), sizeof(uint64_t));
			CopyToVector(data, (const uint8_t*) &entry.metaFilesSizeBytes(), sizeof(uint64_t));
			CopyToVector(data, (const uint8_t*) &entry.replicatorCount(), sizeof(uint16_t));
			CopyToVector(data, (const uint8_t*) &entry.ownerCumulativeUploadSizeBytes(), sizeof(uint64_t));

            SaveActiveDataModifications(entry.activeDataModifications(), data);
            SaveCompletedDataModifications(entry.completedDataModifications(), data);
			SaveConfirmedUsedSizes(entry.confirmedUsedSizes(), data);
			SaveReplicators(entry.replicators(), data);
			SaveReplicators(entry.offboardingReplicators(), data);
            SaveVerifications(entry.verifications(), data);

            return data;
        }

        void AssertCanLoadSingleEntry(VersionType version) {
            // Arrange:
            TestContext context;
            auto originalEntry = CreateBcDriveEntry();
            auto buffer = CreateEntryBuffer(originalEntry, version);

            // Act:
            state::BcDriveEntry result(test::GenerateRandomByteArray<Key>());
            test::RunLoadValueTest<BcDriveEntrySerializer>(buffer, result);

            // Assert:
            test::AssertEqualBcDriveData(originalEntry, result);
        }
    }

    TEST(TEST_CLASS, CanLoadSingleEntry_v1) {
        AssertCanLoadSingleEntry(1);
    }

    // endregion

}}