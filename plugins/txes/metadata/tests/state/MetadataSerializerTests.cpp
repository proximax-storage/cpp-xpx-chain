/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/state/MetadataSerializer.h"
#include "catapult/utils/HexFormatter.h"
#include "tests/test/core/AddressTestUtils.h"
#include "tests/test/core/SerializerOrderingTests.h"
#include "tests/test/core/SerializerTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace state {

#define TEST_CLASS MetadataSerializerTests

    namespace {
        class TestContext {
        public:
            explicit TestContext() : m_stream("", m_buffer)
            {}

        public:
            auto& buffer() {
                return m_buffer;
            }

            auto& outputStream() {
                return m_stream;
            }

        public:
            auto createEntry(model::MetadataType type, size_t fieldCount) {
                auto str = test::GenerateRandomString(15);
                std::vector<uint8_t> buffer(str.size());
                std::copy(str.cbegin(), str.cend(), buffer.begin());
                state::MetadataEntry entry(buffer, type);

                for (size_t i = 0; i < fieldCount; ++i) {
                    auto key = test::GenerateRandomString(20);
                    auto value = test::GenerateRandomString(40);
                    entry.fields().emplace_back(MetadataField{key, value, Height{i}});
                }

                return entry;
            }

        private:
            std::vector<uint8_t> m_buffer;
            mocks::MockMemoryStream m_stream;
        };

        const uint8_t* AssertFieldBuffer(const std::vector<MetadataField>& fields, const uint8_t* pData) {
            auto count = *reinterpret_cast<const uint8_t*>(pData);
            EXPECT_EQ(fields.size(), count);
            pData += sizeof(uint8_t);

            for (const auto field : fields) {
                const auto removeHeight = *reinterpret_cast<const Height*>(pData);
                EXPECT_EQ(field.RemoveHeight.unwrap(), removeHeight.unwrap());
                pData += sizeof(Height);

                auto keySize = size_t{*reinterpret_cast<const uint8_t*>(pData)};
                EXPECT_EQ(field.MetadataKey.size(), keySize);
                pData += sizeof(uint8_t);

                auto key = std::string(pData, pData + keySize);
                EXPECT_EQ(field.MetadataKey, key);
                pData += keySize;

                auto valueSize = size_t{*reinterpret_cast<const uint16_t*>(pData)};
                EXPECT_EQ(field.MetadataValue.size(), valueSize);
                pData += sizeof(uint16_t);

                auto value = std::string(pData, pData + valueSize);
                EXPECT_EQ(field.MetadataValue, value);
                pData += valueSize;
            }

            return pData;
        }

        const uint8_t* AssertEntryBuffer(const state::MetadataEntry& entry, const uint8_t* pData) {
            uint8_t rawIdSize = entry.raw().size();
            EXPECT_EQ(rawIdSize, *reinterpret_cast<const uint8_t*>(pData));
            pData += sizeof(uint8_t);

            std::vector<uint8_t> buffer;
            std::copy(pData, pData + rawIdSize, std::back_inserter(buffer));
            EXPECT_EQ(buffer, entry.raw());
            pData += rawIdSize;

            EXPECT_EQ(entry.type(), *reinterpret_cast<const model::MetadataType*>(pData));
            pData += sizeof(uint8_t);

            return AssertFieldBuffer(entry.fields(), pData);
        }
    }

    // region Save

    TEST(TEST_CLASS, CanSaveSingleEntry) {
        // Arrange:
        TestContext context;
        auto entry = context.createEntry(model::MetadataType::Address, 5);

        // Act:
        MetadataSerializer::Save(entry, context.outputStream());

        // Assert:
        AssertEntryBuffer(entry, context.buffer().data());
    }

    TEST(TEST_CLASS, CanSaveMultipleEntries) {
        // Arrange:
        TestContext context;
        auto entry1 = context.createEntry(model::MetadataType::Address, 10);
        auto entry2 = context.createEntry(model::MetadataType::MosaicId, 15);
        auto entry3 = context.createEntry(model::MetadataType::NamespaceId, 20);

        // Act:
        MetadataSerializer::Save(entry1, context.outputStream());
        MetadataSerializer::Save(entry2, context.outputStream());
        MetadataSerializer::Save(entry3, context.outputStream());

        // Assert:
        const auto* pBuffer = context.buffer().data();
        pBuffer = AssertEntryBuffer(entry1, pBuffer);
        pBuffer = AssertEntryBuffer(entry2, pBuffer);
        pBuffer = AssertEntryBuffer(entry3, pBuffer);
    }

    // endregion

    // region Load

    namespace {
        std::vector<uint8_t> CreateEntryBuffer(const state::MetadataEntry& entry) {
            std::vector<uint8_t> buffer;
            buffer.emplace_back(static_cast<uint8_t>(entry.raw().size()));
            std::copy(entry.raw().begin(), entry.raw().end(), std::back_inserter(buffer));
            buffer.emplace_back(static_cast<uint8_t>(utils::to_underlying_type(entry.type())));

            buffer.emplace_back(static_cast<uint8_t>(entry.fields().size()));
            for (const auto& field : entry.fields()) {
                auto pData = reinterpret_cast<const uint8_t*>(&field.RemoveHeight);
                std::copy(pData, pData + sizeof(Height), std::back_inserter(buffer));

                buffer.emplace_back(static_cast<uint8_t>(field.MetadataKey.size()));

                pData = reinterpret_cast<const uint8_t*>(field.MetadataKey.data());
                std::copy(pData, pData + field.MetadataKey.size(), std::back_inserter(buffer));

                uint16_t valueSize = static_cast<uint16_t>(field.MetadataValue.size());
                pData = reinterpret_cast<const uint8_t*>(&valueSize);
                std::copy(pData, pData + sizeof(uint16_t), std::back_inserter(buffer));

                pData = reinterpret_cast<const uint8_t*>(field.MetadataValue.data());
                std::copy(pData, pData + field.MetadataValue.size(), std::back_inserter(buffer));
            }

            return buffer;
        }

        void AssertEqual(const state::MetadataEntry& expectedEntry, const state::MetadataEntry& entry) {
            EXPECT_EQ(expectedEntry.metadataId(), entry.metadataId());
            EXPECT_EQ(expectedEntry.type(), entry.type());
            EXPECT_EQ(expectedEntry.raw(), entry.raw());
            EXPECT_EQ(expectedEntry.fields().size(), entry.fields().size());
            for (size_t i = 0; i < entry.fields().size(); ++i) {
                EXPECT_EQ(expectedEntry.fields()[i].MetadataKey, entry.fields()[i].MetadataKey);
                EXPECT_EQ(expectedEntry.fields()[i].MetadataValue, entry.fields()[i].MetadataValue);
                EXPECT_EQ(expectedEntry.fields()[i].RemoveHeight.unwrap(), entry.fields()[i].RemoveHeight.unwrap());
            }
        }

        void AssertCanLoadSingleEntry() {
            // Arrange:
            TestContext context;
            auto originalEntry = context.createEntry(model::MetadataType::Address, 5);
            auto buffer = CreateEntryBuffer(originalEntry);
            state::MetadataEntry result;

            // Act:
            test::RunLoadValueTest<MetadataSerializer>(buffer, result);

            // Assert:
            AssertEqual(originalEntry, result);
        }
    }

    TEST(TEST_CLASS, CanLoadSingleEntry) {
        AssertCanLoadSingleEntry();
    }

    // endregion
}}
