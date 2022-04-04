/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/state/SdaExchangeEntrySerializer.h"
#include "tests/test/core/SerializerTestUtils.h"
#include "tests/test/SdaExchangeTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace state {

#define TEST_CLASS SdaExchangeEntrySerializerTests

    namespace {
        constexpr size_t Offer_Count = 2;
        constexpr size_t Expired_Offer_Count = 2;
        constexpr auto Entry_Size = 
            sizeof(VersionType)
            + Key_Size
            + Offer_Count * (sizeof(MosaicsPair) + sizeof(SdaOfferBalance))
            + Expired_Offer_Count * (sizeof(Height) + sizeof(MosaicsPair) + sizeof(SdaOfferBalance));

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

        void AssertSdaOffer(const SdaOfferBalance& offer, const uint8_t*& pData) {
            EXPECT_EQ(offer.CurrentMosaicGive.unwrap(), *reinterpret_cast<const uint64_t*>(pData));
            pData += sizeof(uint64_t);
            EXPECT_EQ(offer.CurrentMosaicGet.unwrap(), *reinterpret_cast<const uint64_t*>(pData));
            pData += sizeof(uint64_t);
            EXPECT_EQ(offer.InitialMosaicGive.unwrap(), *reinterpret_cast<const uint64_t*>(pData));
            pData += sizeof(uint64_t);
            EXPECT_EQ(offer.InitialMosaicGet.unwrap(), *reinterpret_cast<const uint64_t*>(pData));
            pData += sizeof(uint64_t);
            EXPECT_EQ(offer.Deadline.unwrap(), *reinterpret_cast<const uint64_t*>(pData));
            pData += sizeof(uint64_t);
        }

        void AssertSdaOffers(const SdaOfferBalanceMap& offers, const uint8_t*& pData) {
            EXPECT_EQ(offers.size(), *pData);
            pData++;
            for (const auto& pair : offers) {
                EXPECT_EQ(pair.first.first.unwrap(), *reinterpret_cast<const uint64_t*>(pData));
                EXPECT_EQ(pair.first.second.unwrap(), *reinterpret_cast<const uint64_t*>(pData));
                pData += sizeof(uint64_t);
                AssertSdaOffer(pair.second, pData);
            }
        }

        void AssertExpiredSdaOffers(const ExpiredSdaOfferBalanceMap& offers, const uint8_t*& pData) {
            EXPECT_EQ(offers.size(), *reinterpret_cast<const uint16_t*>(pData));
            pData += sizeof(uint16_t);
            for (const auto& pair : offers) {
                EXPECT_EQ(pair.first.unwrap(), *reinterpret_cast<const uint64_t*>(pData));
                pData += sizeof(uint64_t);
                AssertSdaOffers(pair.second, pData);
            }
        }

        void AssertEntryBuffer(const SdaExchangeEntry& entry, const uint8_t* pData, size_t expectedSize) {
            const auto* pExpectedEnd = pData + expectedSize;

            EXPECT_EQ(entry.version(), *reinterpret_cast<const VersionType*>(pData));
            pData += sizeof(VersionType);
            auto owner = Extract<Key_Size>(pData);
            EXPECT_EQ(entry.owner(), owner);
            pData += Key_Size;

            AssertSdaOffers(entry.sdaOfferBalances(), pData);
            AssertExpiredSdaOffers(entry.expiredSdaOfferBalances(), pData);

            EXPECT_EQ(pExpectedEnd, pData);
        }

        template<typename TSerializer>
        void AssertCanSaveSingleEntry(VersionType version) {
            // Arrange:
            TestContext context;
            auto entry = test::CreateSdaExchangeEntry(Offer_Count, Expired_Offer_Count, test::GenerateRandomByteArray<Key>(), version);

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
            auto entry1 = test::CreateSdaExchangeEntry(Offer_Count, Expired_Offer_Count, test::GenerateRandomByteArray<Key>(), version);
            auto entry2 = test::CreateSdaExchangeEntry(Offer_Count, Expired_Offer_Count, test::GenerateRandomByteArray<Key>(), version);

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

        void WriteSdaOffer(const MosaicsPair& pair, const SdaOfferBalance& offer, uint8_t*& pData) {
            auto value = pair.first.unwrap();
            memcpy(pData, &value, sizeof(uint64_t));
            value = pair.second.unwrap();
            memcpy(pData, &value, sizeof(uint64_t));
            pData += sizeof(uint64_t);
            value = offer.CurrentMosaicGive.unwrap();
            memcpy(pData, &value, sizeof(uint64_t));
            pData += sizeof(uint64_t);
            value = offer.CurrentMosaicGet.unwrap();
            memcpy(pData, &value, sizeof(uint64_t));
            pData += sizeof(uint64_t);
            value = offer.InitialMosaicGive.unwrap();
            memcpy(pData, &value, sizeof(uint64_t));
            pData += sizeof(uint64_t);
            value = offer.InitialMosaicGet.unwrap();
            memcpy(pData, &value, sizeof(uint64_t));
            pData += sizeof(uint64_t);
            value = offer.Deadline.unwrap();
            memcpy(pData, &value, sizeof(uint64_t));
            pData += sizeof(uint64_t);
        }

        void WriteSdaOffers(const SdaOfferBalanceMap& offers, uint8_t*& pData) {
            *pData = utils::checked_cast<size_t, uint8_t>(offers.size());
            pData++;
            for (const auto& pair : offers) {
                WriteSdaOffer(pair.first, pair.second, pData);
            }
        }

        void WriteExpiredSdaOffers(const ExpiredSdaOfferBalanceMap& offers, uint8_t*& pData) {
            auto size = utils::checked_cast<size_t, uint16_t>(offers.size());
            memcpy(pData, &size, sizeof(uint16_t));
            pData += sizeof(uint16_t);
            for (const auto& pair : offers) {
                auto value = pair.first.unwrap();
                memcpy(pData, &value, sizeof(uint64_t));
                pData += sizeof(uint64_t);
                WriteSdaOffers(pair.second, pData);
            }
        }

        std::vector<uint8_t> CreateEntryBuffer(const SdaExchangeEntry& entry, size_t expectedSize) {
            std::vector<uint8_t> buffer(expectedSize);

            auto* pData = buffer.data();
            auto version = entry.version();
            memcpy(pData, &version, sizeof(VersionType));
            pData += sizeof(VersionType);
            memcpy(pData, entry.owner().data(), Key_Size);
            pData += Key_Size;

            WriteSdaOffers(entry.sdaOfferBalances(), pData);
            WriteExpiredSdaOffers(entry.expiredSdaOfferBalances(), pData);

            return buffer;
        }

        template<typename TSerializer>
        void AssertCanLoadSingleEntry(VersionType version) {
            // Arrange:
            TestContext context;
            auto originalEntry = test::CreateSdaExchangeEntry(Offer_Count, Expired_Offer_Count, test::GenerateRandomByteArray<Key>(), version);
            auto buffer = CreateEntryBuffer(originalEntry, Entry_Size);

            // Act:
            SdaExchangeEntry result((Key()));
            test::RunLoadValueTest<TSerializer>(buffer, result);

            // Assert:
            test::AssertEqualExchangeData(originalEntry, result);
        }

        // endregion
    }

#define TRAITS_BASED_TEST(TEST_NAME) \
    template<typename TSerializer> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(VersionType version); \
    TEST(TEST_CLASS, TEST_NAME##_v1) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<SdaExchangeEntrySerializer>(1); } \
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
