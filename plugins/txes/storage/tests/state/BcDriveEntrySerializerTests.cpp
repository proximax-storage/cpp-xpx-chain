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

        constexpr auto Active_Data_Modifications_Count = 5;
        constexpr auto Completed_Data_Modifications_Count = 5;

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
                test::Random16(),
                Active_Data_Modifications_Count,
                Completed_Data_Modifications_Count,
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
			EXPECT_EQ(active.ExpectedUploadSize, *reinterpret_cast<const uint64_t*>(pData));
			pData += sizeof(uint64_t);
			EXPECT_EQ(active.ActualUploadSize, *reinterpret_cast<const uint64_t*>(pData));
			pData += sizeof(uint64_t);
			auto folderSize = active.Folder.size();
			EXPECT_EQ(folderSize, *reinterpret_cast<const uint16_t*>(pData));
			pData += sizeof(uint16_t);
			EXPECT_EQ_MEMORY(active.Folder.c_str(), pData, folderSize);
			pData += folderSize;
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
			EXPECT_EQ(entry.usedSize(), *reinterpret_cast<const uint64_t*>(pData));
			pData += sizeof(uint64_t);
			EXPECT_EQ(entry.metaFilesSize(), *reinterpret_cast<const uint64_t*>(pData));
			pData += sizeof(uint64_t);
            EXPECT_EQ(entry.replicatorCount(), *reinterpret_cast<const uint16_t*>(pData));
            pData += sizeof(uint16_t);

            AssertActiveDataModifications(entry.activeDataModifications(), pData);
            AssertCompletedDataModifications(entry.completedDataModifications(), pData);

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
			BcDriveEntrySerializer::Save(entry2, context.outputStream());

			// Assert:
			const auto* pBuffer1 = context.buffer().data();
			const auto* pBuffer2 = pBuffer1 + entry1.size();
			AssertEntryBuffer(entry1, pBuffer1, entry1.size(), version);
			AssertEntryBuffer(entry2, pBuffer2, entry2.size(), version);
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
			CopyToVector(data, (const uint8_t *) &modification.ExpectedUploadSize, sizeof(uint64_t));
			CopyToVector(data, (const uint8_t *) &modification.ActualUploadSize, sizeof(uint64_t));
			auto folderSize = (uint16_t) modification.Folder.size();
			CopyToVector(data, (const uint8_t *) &folderSize, sizeof(folderSize));
			CopyToVector(data, (const uint8_t *) modification.Folder.c_str(), folderSize);
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

        std::vector<uint8_t> CreateEntryBuffer(const state::BcDriveEntry& entry, VersionType version) {
            std::vector<uint8_t> data;
			CopyToVector(data, (const uint8_t*)&version, sizeof(version));
			CopyToVector(data, entry.key().data(), Key_Size);
			CopyToVector(data, entry.owner().data(), Key_Size);
			CopyToVector(data, entry.rootHash().data(), Hash256_Size);
			CopyToVector(data, (const uint8_t*) &entry.size(), sizeof(uint64_t));
			CopyToVector(data, (const uint8_t*) &entry.usedSize(), sizeof(uint64_t));
			CopyToVector(data, (const uint8_t*) &entry.metaFilesSize(), sizeof(uint64_t));
			CopyToVector(data, (const uint8_t*) &entry.replicatorCount(), sizeof(uint16_t));

            SaveActiveDataModifications(entry.activeDataModifications(), data);
            SaveCompletedDataModifications(entry.completedDataModifications(), data);
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