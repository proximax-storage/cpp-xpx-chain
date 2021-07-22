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
        constexpr auto Active_Downloads_Count = 10;
        constexpr auto Completed_Downloads_Count = 10;

        constexpr auto Entry_Size =
            sizeof(VersionType) + // version
            Key_Size + // drive key
            Key_Size + // owner
            Hash256_Size + // root hash
            sizeof(uint64_t) + // size
            sizeof(uint16_t) + // replicator count
            sizeof(uint16_t) + // active data modifications count
            Active_Data_Modifications_Count * (Hash256_Size + Key_Size + Hash256_Size + sizeof(uint64_t)) + // active data modifications
            sizeof(uint16_t) + // completed data modifications count
            Completed_Data_Modifications_Count * (Hash256_Size + Key_Size + Hash256_Size + sizeof(uint64_t) + sizeof(uint8_t)) + // completed data modifications
            sizeof(uint16_t) + // active downloads count
            Active_Downloads_Count * (Hash256_Size) + // active downloads
            sizeof(uint16_t) + // completed downloads count
            Completed_Downloads_Count * (Hash256_Size); // completed downloads
       
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
            return test::CreateBcDriveEntry(
                test::GenerateRandomByteArray<Key>(),
                test::GenerateRandomByteArray<Key>(),
                test::GenerateRandomByteArray<Hash256>(),
                test::Random(),
                test::Random16(),
                Active_Data_Modifications_Count,
                Completed_Data_Modifications_Count,
                Active_Downloads_Count,
                Completed_Downloads_Count);
        }

        void AssertActiveDataModifications(const ActiveDataModifications& activeDataModifications, const uint8_t*& pData) {
            EXPECT_EQ(activeDataModifications.size(), *reinterpret_cast<const uint16_t*>(pData));
            pData += sizeof(uint16_t);
            for (const auto& active : activeDataModifications) {
                EXPECT_EQ_MEMORY(active.Id.data(), pData, Hash256_Size);
                pData += Hash256_Size;
                EXPECT_EQ_MEMORY(active.Owner.data(), pData, Key_Size);
                pData += Key_Size;
                EXPECT_EQ_MEMORY(active.DownloadDataCdi.data(), pData, Hash256_Size);
                pData += Hash256_Size;
                EXPECT_EQ(active.UploadSize, *reinterpret_cast<const uint64_t*>(pData));
                pData += sizeof(uint64_t);
            }
        }

       void AssertCompletedDataModifications(const CompletedDataModifications& completedDataModifications, const uint8_t*& pData) {
            EXPECT_EQ(completedDataModifications.size(), *reinterpret_cast<const uint16_t*>(pData));
            pData += sizeof(uint16_t);
            for (const auto& active : completedDataModifications) {
                EXPECT_EQ_MEMORY(active.Id.data(), pData, Hash256_Size);
                pData += Hash256_Size;
                EXPECT_EQ_MEMORY(active.Owner.data(), pData, Key_Size);
                pData += Key_Size;
                EXPECT_EQ_MEMORY(active.DownloadDataCdi.data(), pData, Hash256_Size);
                pData += Hash256_Size;
                EXPECT_EQ(active.UploadSize, *reinterpret_cast<const uint64_t*>(pData));
                pData += sizeof(uint64_t);
                EXPECT_EQ(active.State, static_cast<DataModificationState>(*pData));
                pData++;
            }
        }

        void AssertActiveDownloads(const std::vector<Hash256> activeDownloads, const uint8_t*& pData) {
            EXPECT_EQ(activeDownloads.size(), *reinterpret_cast<const uint16_t*>(pData));
            pData += sizeof(uint16_t);
            for (const auto& active : activeDownloads) {
                EXPECT_EQ_MEMORY(active.data(), pData, Hash256_Size);
                pData += Hash256_Size;
            }
        }

        void AssertCompletedDownloads(const std::vector<Hash256> completedDownloads, const uint8_t*& pData) {
            EXPECT_EQ(completedDownloads.size(), *reinterpret_cast<const uint16_t*>(pData));
            pData += sizeof(uint16_t);
            for (const auto& completed : completedDownloads) {
                EXPECT_EQ_MEMORY(completed.data(), pData, Hash256_Size);
                pData += Hash256_Size;
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
            EXPECT_EQ(entry.replicatorCount(), *reinterpret_cast<const uint16_t*>(pData));
            pData += sizeof(uint16_t);

            AssertActiveDataModifications(entry.activeDataModifications(), pData);
            AssertCompletedDataModifications(entry.completedDataModifications(), pData);
            AssertActiveDownloads(entry.activeDownloads(), pData);
            AssertCompletedDownloads(entry.completedDownloads(), pData);

            EXPECT_EQ(pExpectedEnd, pData);
        }

		void AssertCanSaveSingleEntry(VersionType version) {
			// Arrange:
			TestContext context;
			auto entry = CreateBcDriveEntry();

			// Act:
			BcDriveEntrySerializer::Save(entry, context.outputStream());

			// Assert:
			ASSERT_EQ(Entry_Size, context.buffer().size());
			AssertEntryBuffer(entry, context.buffer().data(), Entry_Size, version);
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
			ASSERT_EQ(2 * Entry_Size, context.buffer().size());
			const auto* pBuffer1 = context.buffer().data();
			const auto* pBuffer2 = pBuffer1 + Entry_Size;
			AssertEntryBuffer(entry1, pBuffer1, Entry_Size, version);
			AssertEntryBuffer(entry2, pBuffer2, Entry_Size, version);
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
        void SaveActiveDataModifications(const ActiveDataModifications& activeDataModifications, uint8_t*& pData) {
            uint16_t activeDataModificationsCount = utils::checked_cast<size_t, uint16_t>(activeDataModifications.size());
            memcpy(pData, &activeDataModificationsCount, sizeof(uint16_t));
            pData += sizeof(uint16_t);
			for (const auto& modification : activeDataModifications) {
                memcpy(pData, modification.Id.data(), Hash256_Size);
                pData += Hash256_Size;
                memcpy(pData, modification.Owner.data(), Key_Size);
                pData += Key_Size;
                memcpy(pData, modification.DownloadDataCdi.data(), Hash256_Size);
                pData += Hash256_Size;
                memcpy(pData, &modification.UploadSize, sizeof(uint64_t));
                pData += sizeof(uint64_t);
			}
		}

        void SaveCompletedDataModifications(const CompletedDataModifications& completedDataModifications, uint8_t*& pData) {
		    uint16_t completedDataModificationsCount = utils::checked_cast<size_t, uint16_t>(completedDataModifications.size());
            memcpy(pData, &completedDataModificationsCount, sizeof(uint16_t));
            pData += sizeof(uint16_t);
            for (const auto& modification : completedDataModifications) {
                memcpy(pData, modification.Id.data(), Hash256_Size);
                pData += Hash256_Size;
                memcpy(pData, modification.Owner.data(), Key_Size);
                pData += Key_Size;
                memcpy(pData, modification.DownloadDataCdi.data(), Hash256_Size);
                pData += Hash256_Size;
                memcpy(pData, &modification.UploadSize, sizeof(uint64_t));
                pData += sizeof(uint64_t);
                *pData = utils::to_underlying_type(modification.State);
                pData++;
            }
		}

        void SaveActiveDownloads(const std::vector<Hash256> activeDownloads, uint8_t*& pData) {
			uint16_t activeDownloadsCount = utils::checked_cast<size_t, uint16_t>(activeDownloads.size());
            memcpy(pData, &activeDownloadsCount, sizeof(uint16_t));
            pData += sizeof(uint16_t);
			for (const auto& active : activeDownloads) {
				memcpy(pData, active.data(), Hash256_Size);
                pData += Hash256_Size;
			}
		}

		void SaveCompletedDownloads(const std::vector<Hash256> completedDownloads, uint8_t*& pData) {
			uint16_t completedDownloadsCount = utils::checked_cast<size_t, uint16_t>(completedDownloads.size());
            memcpy(pData, &completedDownloadsCount, sizeof(uint16_t));
            pData += sizeof(uint16_t);
            for (const auto& completed : completedDownloads) {
                memcpy(pData, completed.data(), Hash256_Size);
                pData += Hash256_Size;
            }
		}

        std::vector<uint8_t> CreateEntryBuffer(const state::BcDriveEntry& entry, VersionType version) {
            std::vector<uint8_t> buffer(Entry_Size);

            auto* pData = buffer.data();
            memcpy(pData, &version, sizeof(VersionType));
            pData += sizeof(VersionType);
            memcpy(pData, entry.key().data(), Key_Size);
            pData += Key_Size;
            memcpy(pData, entry.owner().data(), Key_Size);
            pData += Key_Size;
            memcpy(pData, entry.rootHash().data(), Hash256_Size);
            pData += Hash256_Size;
            memcpy(pData, &entry.size(), sizeof(uint64_t));
            pData += sizeof(uint64_t);
            memcpy(pData, &entry.replicatorCount(), sizeof(uint16_t));
            pData += sizeof(uint16_t);

            auto activeDataModificationsCount = utils::checked_cast<size_t, uint16_t>(entry.activeDataModifications().size());
            memcpy(pData, &activeDataModificationsCount, sizeof(uint16_t));
            pData += sizeof(uint16_t);
            SaveActiveDataModifications(entry.activeDataModifications(), pData);

            auto completedDataModificationsCount = utils::checked_cast<size_t, uint16_t>(entry.completedDataModifications().size());
            memcpy(pData, &completedDataModificationsCount, sizeof(uint16_t));
            pData += sizeof(uint16_t);
            SaveCompletedDataModifications(entry.completedDataModifications(), pData);

            auto activeDownloadsCount = utils::checked_cast<size_t, uint16_t>(entry.activeDownloads().size());
            memcpy(pData, &activeDownloadsCount, sizeof(uint16_t));
            pData += sizeof(uint16_t);
            SaveActiveDownloads(entry.activeDownloads(), pData);

            auto completedDownloadsCount = utils::checked_cast<size_t, uint16_t>(entry.completedDownloads().size());
            memcpy(pData, &completedDownloadsCount, sizeof(uint16_t));
            pData += sizeof(uint16_t);
            SaveCompletedDownloads(entry.completedDownloads(), pData);

            return buffer;
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