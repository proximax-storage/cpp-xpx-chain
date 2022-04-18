/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/state/SdaOfferGroupEntrySerializer.h"
#include "tests/test/core/SerializerTestUtils.h"
#include "tests/test/SdaExchangeTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace state {

#define TEST_CLASS SdaOfferGroupEntrySerializerTests

    namespace {
        constexpr size_t Offer_Count = 1;
        constexpr auto Entry_Size =
            Hash256_Size
            + Hash256_Size + 2 + Offer_Count * (2 + sizeof(SdaOfferBasicInfo));

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

        template<std::size_t N>
        std::array<uint8_t, N> Extract(const uint8_t* data) {
            std::array<uint8_t, N> buffer = { 0 };
            memcpy(buffer.data(), data, N);
            return buffer;
        }

        // region Save

        void AssertSdaOfferGroup(const SdaOfferGroupVector& info, const uint8_t*& pData) {
            EXPECT_EQ(info.size(), *reinterpret_cast<const uint16_t*>(pData));
            pData += sizeof(uint16_t);
            for (const auto& offer : info) {
                EXPECT_EQ_MEMORY(offer.Owner.data(), pData, Key_Size);
                pData += Key_Size;
                EXPECT_EQ(offer.MosaicGive.unwrap(), *reinterpret_cast<const uint64_t*>(pData));
                pData += sizeof(uint64_t);
                EXPECT_EQ(offer.Deadline.unwrap(), *reinterpret_cast<const uint64_t*>(pData));
                pData += sizeof(uint64_t);
            }
        }

        void AssertEntryBuffer(const SdaOfferGroupEntry& entry, const uint8_t* pData, size_t expectedSize) {
            const auto* pExpectedEnd = pData + expectedSize;

            EXPECT_EQ_MEMORY(entry.groupHash().data(), pData, Hash256_Size);
			pData += Hash256_Size;

            AssertSdaOfferGroup(entry.sdaOfferGroup(), pData);

            EXPECT_EQ(pExpectedEnd, pData);
        }

        template<typename TSerializer>
        void AssertCanSaveSingleEntry(VersionType version) {
            // Arrange:
            TestContext context;
            auto entry = test::CreateSdaOfferGroupEntry(Offer_Count, test::GenerateRandomByteArray<Hash256>());

            // Act:
            TSerializer::Save(entry, context.outputStream());

            // Assert:
            ASSERT_EQ(Entry_Size, context.buffer().size());
            AssertEntryBuffer(entry, context.buffer().data(), Entry_Size);
        }

        template<typename TSerializer>
        void AssertCanSaveMultipleEntries(VersionType version) {
            // Arrange:
            TestContext context;
            auto entry1 = test::CreateSdaOfferGroupEntry(Offer_Count, test::GenerateRandomByteArray<Hash256>());
            auto entry2 = test::CreateSdaOfferGroupEntry(Offer_Count, test::GenerateRandomByteArray<Hash256>());

            // Act:
            TSerializer::Save(entry1, context.outputStream());
            TSerializer::Save(entry2, context.outputStream());

            // Assert:
            ASSERT_EQ(2 * Entry_Size, context.buffer().size());
            const auto* pBuffer1 = context.buffer().data();
            const auto* pBuffer2 = pBuffer1 + Entry_Size;
            AssertEntryBuffer(entry1, pBuffer1, Entry_Size);
            AssertEntryBuffer(entry2, pBuffer2, Entry_Size);
        }

        // endregion

        // region Load

        void WriteSdaOfferGroup(const SdaOfferGroupVector& offers, uint8_t*& pData) {
            uint16_t infoCount = utils::checked_cast<size_t, uint16_t>(offers.size());
            memcpy(pData, &infoCount, sizeof(uint16_t));
			pData += sizeof(uint16_t);
            for (const auto& info : offers) {
                memcpy(pData, info.Owner.data(), Key_Size);
                pData += Key_Size;
                memcpy(pData, &info.MosaicGive, sizeof(uint64_t));
                pData += sizeof(uint64_t);
                memcpy(pData, &info.Deadline, sizeof(uint64_t));
                pData += sizeof(uint64_t);
            }
        }

        std::vector<uint8_t> CreateEntryBuffer(const SdaOfferGroupEntry& entry, size_t expectedSize) {
            std::vector<uint8_t> buffer(expectedSize);

            auto* pData = buffer.data();
            memcpy(pData, entry.groupHash().data(), Hash256_Size);
            pData += Hash256_Size;

            WriteSdaOfferGroup(entry.sdaOfferGroup(), pData);

            return buffer;
        }

        template<typename TSerializer>
        void AssertCanLoadSingleEntry(VersionType version) {
            // Arrange:
            TestContext context;
            auto originalEntry = test::CreateSdaOfferGroupEntry(Offer_Count, test::GenerateRandomByteArray<Hash256>());
            auto buffer = CreateEntryBuffer(originalEntry, Entry_Size);

            // Act:
            SdaOfferGroupEntry result((Hash256()));
            test::RunLoadValueTest<TSerializer>(buffer, result);

            // Assert:
            test::AssertEqualSdaOfferGroupData(originalEntry, result);
        }

        // endregion
    }

#define TRAITS_BASED_TEST(TEST_NAME) \
    template<typename TSerializer> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(VersionType version); \
    TEST(TEST_CLASS, TEST_NAME) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<SdaOfferGroupEntrySerializer>(1); } \
    template<typename TSerializer> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(VersionType version)

    TRAITS_BASED_TEST(CanSaveSingleEntry) {
        AssertCanSaveSingleEntry<TSerializer>(version);
    }

    TRAITS_BASED_TEST(CanSaveMultipleEntries) {
        AssertCanSaveMultipleEntries<TSerializer>(version);
    }

    TRAITS_BASED_TEST(CanLoadSingleEntry) {
        AssertCanLoadSingleEntry<TSerializer>(version);
    }
}}
