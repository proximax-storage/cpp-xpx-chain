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

        constexpr auto Entry_Size_v1 =
            sizeof(VersionType) + // version
            Key_Size + // replicator key
		   	Key_Size + Key_Size + sizeof(uint16_t) + sizeof(uint32_t) + // replicator set node
            sizeof(uint16_t) + // drive count
            Drives_Count * (Key_Size + Hash256_Size + sizeof(uint64_t) + sizeof(uint64_t)) + // drives
		 	sizeof(uint16_t) + // download channel count
			DownloadChannels_Count * Hash256_Size; // download channels

        constexpr auto Entry_Size_v2 =
            sizeof(VersionType) + // version
            Key_Size + // replicator key
            Key_Size + // node boot key
		   	Key_Size + Key_Size + sizeof(uint16_t) + sizeof(uint32_t) + // replicator set node
            sizeof(uint16_t) + // drive count
            Drives_Count * (Key_Size + Hash256_Size + sizeof(uint64_t) + sizeof(uint64_t)) + // drives
		 	sizeof(uint16_t) + // download channel count
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

        auto CreateReplicatorEntry(VersionType version) {
            return test::CreateReplicatorEntry(
                test::GenerateRandomByteArray<Key>(),
				version,
                Drives_Count,
				DownloadChannels_Count);
        }

        void AssertEntryBuffer(const state::ReplicatorEntry& entry, const uint8_t* pData, size_t expectedSize, VersionType version) {
            const auto* pExpectedEnd = pData + expectedSize;
            EXPECT_EQ(version, *reinterpret_cast<const VersionType*>(pData));
            pData += sizeof(VersionType);
            EXPECT_EQ_MEMORY(entry.key().data(), pData, Key_Size);
            pData += Key_Size;

			EXPECT_EQ_MEMORY(entry.replicatorsSetNode().Left.data(), pData, Key_Size);
			pData += Key_Size;
			EXPECT_EQ_MEMORY(entry.replicatorsSetNode().Right.data(), pData, Key_Size);
			pData += Key_Size;
			EXPECT_EQ(entry.replicatorsSetNode().Height, *reinterpret_cast<const uint16_t*>(pData));
			pData += sizeof(uint16_t);
			EXPECT_EQ(entry.replicatorsSetNode().Size, *reinterpret_cast<const uint32_t*>(pData));
			pData += sizeof(uint32_t);

            EXPECT_EQ(entry.drives().size(), *reinterpret_cast<const uint16_t*>(pData));
            pData += sizeof(uint16_t);
            for (const auto& pair : entry.drives()) {
                EXPECT_EQ_MEMORY(pair.first.data(), pData, Key_Size);
                pData += Key_Size;
				const auto& info = pair.second;
				EXPECT_EQ_MEMORY(info.LastApprovedDataModificationId.data(), pData, Hash256_Size);
				pData += Hash256_Size;
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

			if (version > 1) {
                EXPECT_EQ_MEMORY(entry.nodeBootKey().data(), pData, Key_Size);
                pData += Key_Size;
			}

            EXPECT_EQ(pExpectedEnd, pData);
        }

        void AssertCanSaveSingleEntry(VersionType version) {
            // Arrange:
            TestContext context;
            auto entry = CreateReplicatorEntry(version);
			auto expectedSize = (version > 1 ? Entry_Size_v2 : Entry_Size_v1);

            // Act:
            ReplicatorEntrySerializer::Save(entry, context.outputStream());

            // Assert:
            ASSERT_EQ(expectedSize, context.buffer().size());
            AssertEntryBuffer(entry, context.buffer().data(), expectedSize, version);
        }

        void AssertCanSaveMultipleEntries(VersionType version) {
            // Arrange:
            TestContext context;
            auto entry1 = CreateReplicatorEntry(version);
            auto entry2 = CreateReplicatorEntry(version);
			auto expectedSize = (version > 1 ? Entry_Size_v2 : Entry_Size_v1);

            // Act:
            ReplicatorEntrySerializer::Save(entry1, context.outputStream());
            ReplicatorEntrySerializer::Save(entry2, context.outputStream());

            // Assert:
            ASSERT_EQ(2 * expectedSize, context.buffer().size());
            const auto* pBuffer1 = context.buffer().data();
            const auto* pBuffer2 = pBuffer1 + expectedSize;
            AssertEntryBuffer(entry1, pBuffer1, expectedSize, version);
            AssertEntryBuffer(entry2, pBuffer2, expectedSize, version);
        }
    }

    // region Save

    TEST(TEST_CLASS, CanSaveSingleEntry_v1) {
        AssertCanSaveSingleEntry(1);
    }

    TEST(TEST_CLASS, CanSaveMultipleEntries_v1) {
        AssertCanSaveMultipleEntries(1);
    }

    TEST(TEST_CLASS, CanSaveSingleEntry_v2) {
        AssertCanSaveSingleEntry(2);
    }

    TEST(TEST_CLASS, CanSaveMultipleEntries_v2) {
        AssertCanSaveMultipleEntries(2);
    }

    // endregion

    // region Load

    namespace {
        std::vector<uint8_t> CreateEntryBuffer(const state::ReplicatorEntry& entry, VersionType version) {
            std::vector<uint8_t> buffer(version > 1 ? Entry_Size_v2 : Entry_Size_v1);

            auto* pData = buffer.data();
            memcpy(pData, &version, sizeof(VersionType));
            pData += sizeof(VersionType);
            memcpy(pData, entry.key().data(), Key_Size);
            pData += Key_Size;

			memcpy(pData, entry.replicatorsSetNode().Left.data(), Key_Size);
			pData += Key_Size;
			memcpy(pData, entry.replicatorsSetNode().Right.data(), Key_Size);
			pData += Key_Size;
			memcpy(pData, &entry.replicatorsSetNode().Height, sizeof(uint16_t));
			pData += sizeof(uint16_t);
			memcpy(pData, &entry.replicatorsSetNode().Size, sizeof(uint32_t));
			pData += sizeof(uint32_t);

            uint16_t drivesCount = utils::checked_cast<size_t, uint16_t>(entry.drives().size());
            memcpy(pData, &drivesCount, sizeof(uint16_t));
            pData += sizeof(uint16_t);
            for (const auto& pair : entry.drives()) {
                memcpy(pData, pair.first.data(), Key_Size);
                pData += Key_Size;
				const auto& info = pair.second;
				memcpy(pData, info.LastApprovedDataModificationId.data(), Hash256_Size);
				pData += Hash256_Size;
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

			if (version > 1) {
				memcpy(pData, entry.nodeBootKey().data(), Key_Size);
				pData += Key_Size;
			}

            return buffer;
        }

        void AssertCanLoadSingleEntry(VersionType version) {
            // Arrange:
            TestContext context;
            auto originalEntry = CreateReplicatorEntry(version);
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

    TEST(TEST_CLASS, CanLoadSingleEntry_v2) {
        AssertCanLoadSingleEntry(2);
    }

    // endregion

}}