/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "tests/test/core/SerializerTestUtils.h"
#include "tests/test/StorageTestUtils.h"

namespace catapult { namespace state {

#define TEST_CLASS ReplicatorEntrySerializerTests

	namespace {

        constexpr auto Drives_Count = 5;
        constexpr auto DownloadChannels_Count = 3;

        constexpr auto Entry_Size =
            sizeof(VersionType) + // version
            Key_Size + // drive key
            sizeof(Amount) + // capacity
            sizeof(uint16_t) + // drive count
            Drives_Count * (Key_Size + Hash256_Size + sizeof(bool) + sizeof(uint64_t) + sizeof(uint64_t)) + // drives
			DownloadChannels_Count * Hash256_Size; // download channels

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

        auto CreateReplicatorEntry() {
            return test::CreateReplicatorEntry(
                test::GenerateRandomByteArray<Key>(),
                test::GenerateRandomValue<Amount>(),
                Drives_Count,
				DownloadChannels_Count);
        }

        void AssertEntryBuffer(const state::ReplicatorEntry& entry, const uint8_t* pData, size_t expectedSize, VersionType version) {
            const auto* pExpectedEnd = pData + expectedSize;
            EXPECT_EQ(version, *reinterpret_cast<const VersionType*>(pData));
            pData += sizeof(VersionType);
            EXPECT_EQ_MEMORY(entry.key().data(), pData, Key_Size);
            pData += Key_Size;
            EXPECT_EQ(entry.capacity(), *reinterpret_cast<const Amount*>(pData));
            pData += sizeof(Amount);

            EXPECT_EQ(entry.drives().size(), *reinterpret_cast<const uint16_t*>(pData));
            pData += sizeof(uint16_t);
            for (const auto& pair : entry.drives()) {
                EXPECT_EQ_MEMORY(pair.first.data(), pData, Key_Size);
                pData += Key_Size;
				const auto& info = pair.second;
				EXPECT_EQ_MEMORY(info.LastApprovedDataModificationId.data(), pData, Hash256_Size);
				pData += Hash256_Size;
				EXPECT_EQ(info.DataModificationIdIsValid, *reinterpret_cast<const bool*>(pData));
				pData += sizeof(bool);
				EXPECT_EQ(info.InitialDownloadWorkMegabytes, *reinterpret_cast<const uint64_t*>(pData));
				pData += sizeof(uint64_t);
                EXPECT_EQ(info.LastCompletedCumulativeDownloadWorkBytes, *reinterpret_cast<const uint64_t*>(pData));
                pData += sizeof(uint64_t);
            }

            EXPECT_EQ(entry.downloadChannels().size(), *reinterpret_cast<const uint16_t*>(pData));
            pData += sizeof(uint16_t);
            for (const auto& id : entry.downloadChannels()) {
                EXPECT_EQ_MEMORY(id.data(), pData, Hash256_Size);
                pData += Hash256_Size;
            }

            EXPECT_EQ(pExpectedEnd, pData);
        }

        void AssertCanSaveSingleEntry(VersionType version) {
            // Arrange:
            TestContext context;
            auto entry = CreateReplicatorEntry();

            // Act:
            ReplicatorEntrySerializer::Save(entry, context.outputStream());

            // Assert:
            ASSERT_EQ(Entry_Size, context.buffer().size());
            AssertEntryBuffer(entry, context.buffer().data(), Entry_Size, version);
        }

        void AssertCanSaveMultipleEntries(VersionType version) {
            // Arrange:
            TestContext context;
            auto entry1 = CreateReplicatorEntry();
            auto entry2 = CreateReplicatorEntry();

            // Act:
            ReplicatorEntrySerializer::Save(entry1, context.outputStream());
            ReplicatorEntrySerializer::Save(entry2, context.outputStream());

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
        std::vector<uint8_t> CreateEntryBuffer(const state::ReplicatorEntry& entry, VersionType version) {
            std::vector<uint8_t> buffer(Entry_Size);

            auto* pData = buffer.data();
            memcpy(pData, &version, sizeof(VersionType));
            pData += sizeof(VersionType);
            memcpy(pData, entry.key().data(), Key_Size);
            pData += Key_Size;
            memcpy(pData, &entry.capacity(), sizeof(Amount));
            pData += sizeof(Amount);

            uint16_t drivesCount = utils::checked_cast<size_t, uint16_t>(entry.drives().size());
            memcpy(pData, &drivesCount, sizeof(uint16_t));
            pData += sizeof(uint16_t);
            for (const auto& pair : entry.drives()) {
                memcpy(pData, pair.first.data(), Key_Size);
                pData += Key_Size;
				const auto& info = pair.second;
				memcpy(pData, info.LastApprovedDataModificationId.data(), Hash256_Size);
				pData += Hash256_Size;
				memcpy(pData, &info.DataModificationIdIsValid, sizeof(bool));
				pData += sizeof(bool);
				memcpy(pData, &info.InitialDownloadWorkMegabytes, sizeof(uint64_t));
				pData += sizeof(uint64_t);
                memcpy(pData, &info.LastCompletedCumulativeDownloadWorkBytes, sizeof(uint64_t));
                pData += sizeof(uint64_t);
            }

            uint16_t downloadChannelCount = utils::checked_cast<size_t, uint16_t>(entry.downloadChannels().size());
            memcpy(pData, &downloadChannelCount, sizeof(uint16_t));
            pData += sizeof(uint16_t);
            for (const auto& id : entry.downloadChannels()) {
                memcpy(pData, id.data(), Hash256_Size);
                pData += Hash256_Size;
            }

            return buffer;
        }

        void AssertCanLoadSingleEntry(VersionType version) {
            // Arrange:
            TestContext context;
            auto originalEntry = CreateReplicatorEntry();
            auto buffer = CreateEntryBuffer(originalEntry, version);

            // Act:
            state::ReplicatorEntry result(test::GenerateRandomByteArray<Key>());
            test::RunLoadValueTest<ReplicatorEntrySerializer>(buffer, result);

            // Assert:
            test::AssertEqualReplicatorData(originalEntry, result);
        }
    }

    TEST(TEST_CLASS, CanLoadSingleEntry_v1) {
        AssertCanLoadSingleEntry(1);
    }

    // endregion

}}