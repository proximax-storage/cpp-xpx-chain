/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/state/BootKeyReplicatorEntrySerializer.h"
#include "tests/test/core/SerializerTestUtils.h"
#include "tests/test/StorageTestUtils.h"

namespace catapult { namespace state {

#define TEST_CLASS BootKeyReplicatorEntrySerializerTests

	namespace {
        constexpr auto Entry_Size =
            sizeof(VersionType) + // version
            Key_Size + // node boot key
		   	Key_Size; // replicator key

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

        void AssertEntryBuffer(const state::BootKeyReplicatorEntry& entry, const uint8_t* pData, size_t expectedSize, VersionType version) {
            const auto* pExpectedEnd = pData + expectedSize;
            EXPECT_EQ(version, *reinterpret_cast<const VersionType*>(pData));
            pData += sizeof(VersionType);
            EXPECT_EQ_MEMORY(entry.nodeBootKey().data(), pData, Key_Size);
            pData += Key_Size;
            EXPECT_EQ_MEMORY(entry.replicatorKey().data(), pData, Key_Size);
            pData += Key_Size;

            EXPECT_EQ(pExpectedEnd, pData);
        }

        void AssertCanSaveSingleEntry(VersionType version) {
            // Arrange:
            TestContext context;
			auto entry = test::CreateBootKeyReplicatorEntry();

            // Act:
            BootKeyReplicatorEntrySerializer::Save(entry, context.outputStream());

            // Assert:
            ASSERT_EQ(Entry_Size, context.buffer().size());
            AssertEntryBuffer(entry, context.buffer().data(), Entry_Size, version);
        }

        void AssertCanSaveMultipleEntries(VersionType version) {
            // Arrange:
            TestContext context;
			auto entry1 = test::CreateBootKeyReplicatorEntry();
			auto entry2 = test::CreateBootKeyReplicatorEntry();

            // Act:
            BootKeyReplicatorEntrySerializer::Save(entry1, context.outputStream());
            BootKeyReplicatorEntrySerializer::Save(entry2, context.outputStream());

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
        std::vector<uint8_t> CreateEntryBuffer(const state::BootKeyReplicatorEntry& entry, VersionType version) {
            std::vector<uint8_t> buffer(Entry_Size);

            auto* pData = buffer.data();
            memcpy(pData, &version, sizeof(VersionType));
            pData += sizeof(VersionType);
            memcpy(pData, entry.nodeBootKey().data(), Key_Size);
            pData += Key_Size;
            memcpy(pData, entry.replicatorKey().data(), Key_Size);
            pData += Key_Size;

            return buffer;
        }

        void AssertCanLoadSingleEntry(VersionType version) {
            // Arrange:
            TestContext context;
			auto originalEntry = test::CreateBootKeyReplicatorEntry();
            auto buffer = CreateEntryBuffer(originalEntry, version);

            // Act:
            state::BootKeyReplicatorEntry result(test::GenerateRandomByteArray<Key>(), test::GenerateRandomByteArray<Key>());
            test::RunLoadValueTest<BootKeyReplicatorEntrySerializer>(buffer, result);

            // Assert:
            test::AssertEqualBootKeyReplicatorData(originalEntry, result);
        }
    }

    TEST(TEST_CLASS, CanLoadSingleEntry_v1) {
        AssertCanLoadSingleEntry(1);
    }

    // endregion

}}