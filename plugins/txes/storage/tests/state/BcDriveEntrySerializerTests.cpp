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

        constexpr auto Replicator_Count = 5;
        constexpr auto Active_Data_Modifications_Count = 5;
        constexpr auto Completed_Data_Modifications_Count = 5;
        constexpr auto Active_Downloads_Count = 10;
        constexpr auto Completed_Downloads_Count = 10;

        constexpr auto Entry_Size =
            sizeof(VersionType) + // version
            Key_Size + // drive key
            Key_Size + // owner
            Hash256_Size + // root hash
            sizeof(uint16_t) + // size
            sizeof(uint64_t) + // replicator count
            Active_Data_Modifications_Count * (Hash256_Size + Key_Size + Hash256_Size + sizeof(uint64_t)) + // active data modifications
            Completed_Data_Modifications_Count * (Hash256_Size + Key_Size + Hash256_Size + sizeof(uint64_t) + sizeof(uint8_t)) + // completed data modifications
            Active_Downloads_Count * (Hash256_Size) + // active downloads
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
                Replicator_Count,
                Active_Data_Modifications_Count,
                Completed_Data_Modifications_Count,
                Active_Downloads_Count,
                Completed_Downloads_Count);
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

            //region active data modifications
            
            EXPECT_EQ(entry.activeDataModifications().size(), *reinterpret_cast<const uint16_t*>(pData));
            pData += sizeof(uint16_t);
            for (const auto& details : entry.activeDataModifications()) {
                EXPECT_EQ_MEMORY(details.Id.data(), pData, Hash256_Size);
                pData += Hash256_Size;
                EXPECT_EQ_MEMORY(details.Owner.data(), pData, Key_Size);
                pData += Key_Size;
                EXPECT_EQ_MEMORY(details.DownloadDataCdi.data(), pData, Hash256_Size);
                pData += Hash256_Size;
                EXPECT_EQ(details.UploadSize, *reinterpret_cast<const uint64_t*>(pData));
                pData += sizeof(uint64_t);
            }

            // end region

            //region completed data modifications
            
            EXPECT_EQ(entry.completedDataModifications().size(), *reinterpret_cast<const uint16_t*>(pData));
            pData += sizeof(uint16_t);
            for (const auto& details : entry.completedDataModifications()) {
                EXPECT_EQ_MEMORY(details.Id.data(), pData, Hash256_Size);
                pData += Hash256_Size;
                EXPECT_EQ_MEMORY(details.Owner.data(), pData, Key_Size);
                pData += Key_Size;
                EXPECT_EQ_MEMORY(details.DownloadDataCdi.data(), pData, Hash256_Size);
                pData += Hash256_Size;
                EXPECT_EQ(details.UploadSize, *reinterpret_cast<const uint64_t*>(pData));
                pData += sizeof(uint64_t);
                EXPECT_EQ(details.State, *reinterpret_cast<const uint8_t*>(pData));
                pData += sizeof(uint8_t);
            }

            // end region

            //region active downloads
            
            EXPECT_EQ(entry.activeDownloads().size(), *reinterpret_cast<const uint16_t*>(pData));
            pData += sizeof(uint16_t);
            for (const auto& details : entry.activeDownloads()) {
                EXPECT_EQ_MEMORY(details.data(), pData, Hash256_Size);
            }

            // end region

            //region completed downloads
            
            EXPECT_EQ(entry.completedDownloads().size(), *reinterpret_cast<const uint16_t*>(pData));
            pData += sizeof(uint16_t);
            for (const auto& details : entry.completedDownloads()) {
                EXPECT_EQ_MEMORY(details.data(), pData, Hash256_Size);
            }

            // end region

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
            for (const auto& details : entry.activeDataModifications()) {
                memcpy(pData, &details.Id, Hash256_Size);
                pData += Hash256_Size;
                memcpy(pData, &details.Owner, Key_Size);
                pData += Key_Size;
                memcpy(pData, &details.DownloadDataCdi, Hash256_Size);
                pData += Hash256_Size;
                memcpy(pData, &details.UploadSize, sizeof(uint64_t));
                pData += sizeof(uint64_t);
            }

            auto completedDataModificationsCount = utils::checked_cast<size_t, uint16_t>(entry.completedDataModifications().size());
            memcpy(pData, &completedDataModificationsCount, sizeof(uint16_t));
            pData += sizeof(uint16_t);
            for (const auto& details : entry.completedDataModifications()) {
                memcpy(pData, &details.Id, Hash256_Size);
                pData += Hash256_Size;
                memcpy(pData, &details.Owner, Key_Size);
                pData += Key_Size;
                memcpy(pData, &details.DownloadDataCdi, Hash256_Size);
                pData += Hash256_Size;
                memcpy(pData, &details.UploadSize, sizeof(uint64_t));
                pData += sizeof(uint64_t);
                memcpy(pData, &details.State, sizeof(uint8_t));
                pData += sizeof(uint8_t);
            }

            auto activeDownloadsCount = utils::checked_cast<size_t, uint16_t>(entry.activeDownloads().size());
            memcpy(pData, &activeDownloadsCount, sizeof(uint16_t));
            pData += sizeof(uint16_t);
            for (const auto& details : entry.activeDownloads()) {
                memcpy(pData, &details, Hash256_Size);
                pData += Hash256_Size;
            }

            auto completedDownloadsCount = utils::checked_cast<size_t, uint16_t>(entry.completedDownloads().size());
            memcpy(pData, &completedDownloadsCount, sizeof(uint16_t));
            pData += sizeof(uint16_t);
            for (const auto& details : entry.completedDownloads()) {
                memcpy(pData, &details, Hash256_Size);
                pData += Hash256_Size;
            }

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