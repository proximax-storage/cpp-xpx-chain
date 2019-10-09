//
// Created by ruell on 09/10/2019.
//

#include "src/state/HelloEntrySerializer.h"
#include "catapult/utils/HexFormatter.h"
#include "tests/test/core/AddressTestUtils.h"
#include "tests/test/core/SerializerOrderingTests.h"
#include "tests/test/core/SerializerTestUtils.h"
#include "tests/TestHarness.h"
#include "tests/test/HelloTestUtils.h"

namespace catapult { namespace state {

#define TEST_CLASS HelloEntrySerializerTests

        namespace {
            constexpr auto Entry_Size = sizeof(Key) + sizeof(uint16_t) + sizeof(uint32_t); // in HelloEntrySerializer we write version first, 32bits

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

		    Key ExtractKey(const uint8_t* data) {
                Key key;
                memcpy(key.data(), data, Key_Size);
                return key;
            }
#if 1
            void AssertEntryBuffer(const state::HelloEntry& entry, const uint8_t* pData, size_t expectedSize, VersionType version) {
                const auto* pExpectedEnd = pData + expectedSize;

                // Should be same sequence as to how serializer wrote it
                EXPECT_EQ(version, *reinterpret_cast<const VersionType*>(pData));
                pData += sizeof(VersionType);              // version written by serializer

                auto accountKey = ExtractKey(pData);
                EXPECT_EQ(entry.key(), accountKey);
                pData += Key_Size;

                EXPECT_EQ(entry.messageCount(), *reinterpret_cast<const uint16_t*>(pData));
                pData += sizeof(uint16_t);

                EXPECT_EQ(pExpectedEnd, pData);
            }
#endif

            void AssertEqual(const state::HelloEntry& expectedEntry, const state::HelloEntry& entry) {
                EXPECT_EQ(expectedEntry.messageCount(), entry.messageCount());
                EXPECT_EQ(expectedEntry.key(), entry.key());
            }

            void AssertCanSaveSingleEntry(VersionType version) {
                // Arrange:
                TestContext context;
                auto key = test::GenerateKeys(1);
                auto entry = state::HelloEntry( key[0], 5);

                // Act:
                HelloEntrySerializer::Save(entry, context.outputStream());

                // Assert:
                ASSERT_EQ(Entry_Size, context.buffer().size());
                AssertEntryBuffer(entry, context.buffer().data(), Entry_Size, version);
            }

            void AssertCanSaveMultipleEntries(VersionType version) {
                // Arrange:
                TestContext context;
                auto key = test::GenerateKeys(2);
                auto entry1 = state::HelloEntry(key[0], 1);
                auto entry2 = state::HelloEntry(key[1], 2);

                // Act:
                HelloEntrySerializer::Save(entry1, context.outputStream());
                HelloEntrySerializer::Save(entry2, context.outputStream());

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

            std::vector<uint8_t> CreateEntryBuffer(const state::HelloEntry& entry, VersionType version) {
                std::vector<uint8_t> buffer(Entry_Size);

                // order must be in same sequence
                auto* pData = buffer.data();
                memcpy(pData, &version, sizeof(VersionType));
                pData += sizeof(VersionType);

                auto key = entry.key();
                memcpy(pData, &key, Key_Size);
                pData += Key_Size;

                auto messageCount = entry.messageCount();
                memcpy(pData, &messageCount, sizeof(uint16_t));
                pData += sizeof(uint16_t);

                return buffer;
            }

            void AssertCanLoadSingleEntry(VersionType version) {
                // Arrange:
                TestContext context;
                auto key = test::GenerateKeys(1);
                auto originalEntry = state::HelloEntry(key[0], 1);
                auto buffer = CreateEntryBuffer(originalEntry, version);

                // Act:
                state::HelloEntry result;
                test::RunLoadValueTest<HelloEntrySerializer>(buffer, result);

                // Assert:
                AssertEqual(originalEntry, result);
            }
        }

        TEST(TEST_CLASS, CanLoadSingleEntry_v1) {
            AssertCanLoadSingleEntry(1);
        }

        // endregion
    }}
