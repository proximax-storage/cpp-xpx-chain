/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "tests/test/core/SerializerTestUtils.h"
#include "tests/test/ExchangeTestUtils.h"

namespace catapult { namespace state {

#define TEST_CLASS ExchangeEntrySerializerTests

	namespace {
		constexpr size_t Offer_Count = 2;
		constexpr size_t Expired_Offer_Count = 2;
		constexpr auto Entry_Size_Without_History =
			sizeof(VersionType)
			+ Key_Size
			+ 1 + Offer_Count * (sizeof(MosaicId) + sizeof(OfferBase))
			+ 1 + Offer_Count * (sizeof(MosaicId) + sizeof(OfferBase) + sizeof(Amount));
		constexpr auto Entry_Size_With_History =
			Entry_Size_Without_History
			+ 2 + Expired_Offer_Count * (sizeof(Height) + 1 + sizeof(MosaicId) + sizeof(OfferBase))
			+ 2 + Expired_Offer_Count * (sizeof(Height) + 1 + sizeof(MosaicId) + sizeof(OfferBase) + sizeof(Amount));

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

		void AssertOfferBase(const OfferBase& offer, const uint8_t*& pData) {
			EXPECT_EQ(offer.Amount.unwrap(), *reinterpret_cast<const uint64_t*>(pData));
			pData += sizeof(uint64_t);
			EXPECT_EQ(offer.InitialAmount.unwrap(), *reinterpret_cast<const uint64_t*>(pData));
			pData += sizeof(uint64_t);
			EXPECT_EQ(offer.InitialCost.unwrap(), *reinterpret_cast<const uint64_t*>(pData));
			pData += sizeof(uint64_t);
			EXPECT_EQ(offer.Deadline.unwrap(), *reinterpret_cast<const uint64_t*>(pData));
			pData += sizeof(uint64_t);
		}

		void AssertSellOffers(const SellOfferMap& offers, const uint8_t*& pData) {
			EXPECT_EQ(offers.size(), *pData);
			pData++;
			for (const auto& pair : offers) {
				EXPECT_EQ(pair.first.unwrap(), *reinterpret_cast<const uint64_t*>(pData));
				pData += sizeof(uint64_t);
				AssertOfferBase(pair.second, pData);
			}
		}

		void AssertBuyOffers(const BuyOfferMap& offers, const uint8_t*& pData) {
			EXPECT_EQ(offers.size(), *pData);
			pData++;
			for (const auto& pair : offers) {
				EXPECT_EQ(pair.first.unwrap(), *reinterpret_cast<const uint64_t*>(pData));
				pData += sizeof(uint64_t);
				auto& offer = pair.second;
				AssertOfferBase(offer, pData);
				EXPECT_EQ(offer.ResidualCost.unwrap(), *reinterpret_cast<const uint64_t*>(pData));
				pData += sizeof(uint64_t);
			}
		}

		void AssertExpiredSellOffers(const ExpiredSellOfferMap& offers, const uint8_t*& pData) {
			EXPECT_EQ(offers.size(), *reinterpret_cast<const uint16_t*>(pData));
			pData += sizeof(uint16_t);
			for (const auto& pair : offers) {
				EXPECT_EQ(pair.first.unwrap(), *reinterpret_cast<const uint64_t*>(pData));
				pData += sizeof(uint64_t);
				AssertSellOffers(pair.second, pData);
			}
		}

		void AssertExpiredBuyOffers(const ExpiredBuyOfferMap& offers, const uint8_t*& pData) {
			EXPECT_EQ(offers.size(), *reinterpret_cast<const uint16_t*>(pData));
			pData += sizeof(uint16_t);
			for (const auto& pair : offers) {
				EXPECT_EQ(pair.first.unwrap(), *reinterpret_cast<const uint64_t*>(pData));
				pData += sizeof(uint64_t);
				AssertBuyOffers(pair.second, pData);
			}
		}

		void AssertEntryBuffer(const ExchangeEntry& entry, const uint8_t* pData, size_t expectedSize, bool assertHistory) {
			const auto* pExpectedEnd = pData + expectedSize;

			EXPECT_EQ(entry.version(), *reinterpret_cast<const VersionType*>(pData));
			pData += sizeof(VersionType);
			auto owner = Extract<Key_Size>(pData);
			EXPECT_EQ(entry.owner(), owner);
			pData += Key_Size;

			AssertSellOffers(entry.sellOffers(), pData);
			AssertBuyOffers(entry.buyOffers(), pData);

			if (assertHistory) {
				AssertExpiredSellOffers(entry.expiredSellOffers(), pData);
				AssertExpiredBuyOffers(entry.expiredBuyOffers(), pData);
			}

			EXPECT_EQ(pExpectedEnd, pData);
		}

		template<typename TTraits, typename TSerializer>
		void AssertCanSaveSingleEntry(VersionType version) {
			// Arrange:
			TestContext context;
			auto entry = test::CreateExchangeEntry(Offer_Count, Expired_Offer_Count, test::GenerateRandomByteArray<Key>(), version);

			// Act:
			TSerializer::Save(entry, context.outputStream());

			// Assert:
			ASSERT_EQ(TTraits::Entry_Size, context.buffer().size());
			TTraits::AssertBuffer(entry, context.buffer().data());
		}

		template<typename TTraits, typename TSerializer>
		void AssertCanSaveMultipleEntries(VersionType version) {
			// Arrange:
			TestContext context;
			auto entry1 = test::CreateExchangeEntry(Offer_Count, Expired_Offer_Count, test::GenerateRandomByteArray<Key>(), version);
			auto entry2 = test::CreateExchangeEntry(Offer_Count, Expired_Offer_Count, test::GenerateRandomByteArray<Key>(), version);

			// Act:
			TSerializer::Save(entry1, context.outputStream());
			TSerializer::Save(entry2, context.outputStream());

			// Assert:
			ASSERT_EQ(2 * TTraits::Entry_Size, context.buffer().size());
			const auto* pBuffer1 = context.buffer().data();
			const auto* pBuffer2 = pBuffer1 + TTraits::Entry_Size;
			TTraits::AssertBuffer(entry1, pBuffer1);
			TTraits::AssertBuffer(entry2, pBuffer2);
		}

		// endregion

		// region Load

		void WriteOfferBase(const MosaicId& mosaicId, const OfferBase& offer, uint8_t*& pData) {
			auto value = mosaicId.unwrap();
			memcpy(pData, &value, sizeof(uint64_t));
			pData += sizeof(uint64_t);
			value = offer.Amount.unwrap();
			memcpy(pData, &value, sizeof(uint64_t));
			pData += sizeof(uint64_t);
			value = offer.InitialAmount.unwrap();
			memcpy(pData, &value, sizeof(uint64_t));
			pData += sizeof(uint64_t);
			value = offer.InitialCost.unwrap();
			memcpy(pData, &value, sizeof(uint64_t));
			pData += sizeof(uint64_t);
			value = offer.Deadline.unwrap();
			memcpy(pData, &value, sizeof(uint64_t));
			pData += sizeof(uint64_t);
		}

		void WriteSellOffers(const SellOfferMap& offers, uint8_t*& pData) {
			*pData = utils::checked_cast<size_t, uint8_t>(offers.size());
			pData++;
			for (const auto& pair : offers) {
				WriteOfferBase(pair.first, pair.second, pData);
			}
		}

		void WriteBuyOffers(const BuyOfferMap& offers, uint8_t*& pData) {
			*pData = utils::checked_cast<size_t, uint8_t>(offers.size());
			pData++;
			for (const auto& pair : offers) {
				WriteOfferBase(pair.first, pair.second, pData);
				auto value = pair.second.ResidualCost.unwrap();
				memcpy(pData, &value, sizeof(uint64_t));
				pData += sizeof(uint64_t);
			}
		}

		void WriteExpiredSellOffers(const ExpiredSellOfferMap& offers, uint8_t*& pData) {
			auto size = utils::checked_cast<size_t, uint16_t>(offers.size());
			memcpy(pData, &size, sizeof(uint16_t));
			pData += sizeof(uint16_t);
			for (const auto& pair : offers) {
				auto value = pair.first.unwrap();
				memcpy(pData, &value, sizeof(uint64_t));
				pData += sizeof(uint64_t);
				WriteSellOffers(pair.second, pData);
			}
		}

		void WriteExpiredBuyOffers(const ExpiredBuyOfferMap& offers, uint8_t*& pData) {
			auto size = utils::checked_cast<size_t, uint16_t>(offers.size());
			memcpy(pData, &size, sizeof(uint16_t));
			pData += sizeof(uint16_t);
			for (const auto& pair : offers) {
				auto value = pair.first.unwrap();
				memcpy(pData, &value, sizeof(uint64_t));
				pData += sizeof(uint64_t);
				WriteBuyOffers(pair.second, pData);
			}
		}

		std::vector<uint8_t> CreateEntryBuffer(const ExchangeEntry& entry, size_t expectedSize, bool writeHistory) {
			std::vector<uint8_t> buffer(expectedSize);

			auto* pData = buffer.data();
			auto version = entry.version();
			memcpy(pData, &version, sizeof(VersionType));
			pData += sizeof(VersionType);
			memcpy(pData, entry.owner().data(), Key_Size);
			pData += Key_Size;

			WriteSellOffers(entry.sellOffers(), pData);
			WriteBuyOffers(entry.buyOffers(), pData);

			if (writeHistory) {
				WriteExpiredSellOffers(entry.expiredSellOffers(), pData);
				WriteExpiredBuyOffers(entry.expiredBuyOffers(), pData);
			}

			return buffer;
		}

		template<typename TTraits, typename TSerializer>
		void AssertCanLoadSingleEntry(VersionType version) {
			// Arrange:
			TestContext context;
			auto originalEntry = test::CreateExchangeEntry(Offer_Count, Expired_Offer_Count, test::GenerateRandomByteArray<Key>(), version);
			auto buffer = TTraits::CreateBuffer(originalEntry);
			TTraits::AdaptEntry(originalEntry);

			// Act:
			ExchangeEntry result((Key()));
			test::RunLoadValueTest<TSerializer>(buffer, result);

			// Assert:
			test::AssertEqualExchangeData(originalEntry, result);
		}

		// endregion

		struct NonHistoricalTraits {
		public:
			static constexpr auto Entry_Size = Entry_Size_Without_History;

			static void AssertBuffer(const ExchangeEntry& entry, const uint8_t* pData) {
				AssertEntryBuffer(entry, pData, Entry_Size_Without_History, false);
			}

			static std::vector<uint8_t> CreateBuffer(const ExchangeEntry& entry) {
				return CreateEntryBuffer(entry, Entry_Size_Without_History, false);
			}

			static void AdaptEntry(ExchangeEntry& entry) {
				entry.expiredSellOffers().clear();
				entry.expiredBuyOffers().clear();
			}
		};

		struct HistoricalTraits {
		public:
			static constexpr auto Entry_Size = Entry_Size_With_History;

			static void AssertBuffer(const ExchangeEntry& entry, const uint8_t* pData) {
				AssertEntryBuffer(entry, pData, Entry_Size_With_History, true);
			}

			static std::vector<uint8_t> CreateBuffer(const ExchangeEntry& entry) {
				return CreateEntryBuffer(entry, Entry_Size_With_History, true);
			}

			static void AdaptEntry(ExchangeEntry&) {
			}
		};
	}

#define TRAITS_BASED_TEST(TEST_NAME) \
	template<typename TTraits, typename TSerializer> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(VersionType version); \
	TEST(TEST_CLASS, TEST_NAME##_NonHistorical_v1) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<HistoricalTraits, ExchangeEntryNonHistoricalSerializer>(1); } \
	TEST(TEST_CLASS, TEST_NAME##_Historical_v1) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<HistoricalTraits, ExchangeEntrySerializer>(1); } \
	TEST(TEST_CLASS, TEST_NAME##_NonHistorical_v2) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<NonHistoricalTraits, ExchangeEntryNonHistoricalSerializer>(2); } \
	TEST(TEST_CLASS, TEST_NAME##_Historical_v2) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<HistoricalTraits, ExchangeEntrySerializer>(2); } \
	TEST(TEST_CLASS, TEST_NAME##_NonHistorical_v3) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<NonHistoricalTraits, ExchangeEntryNonHistoricalSerializer>(3); } \
	TEST(TEST_CLASS, TEST_NAME##_Historical_v3) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<HistoricalTraits, ExchangeEntrySerializer>(3); } \
	template<typename TTraits, typename TSerializer> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(VersionType version)

	TRAITS_BASED_TEST(CanSaveSingleEntry) {
		AssertCanSaveSingleEntry<TTraits, TSerializer>(version);
	}

	TRAITS_BASED_TEST(CanSaveMultipleEntries) {
		AssertCanSaveMultipleEntries<TTraits, TSerializer>(version);
	}

	TRAITS_BASED_TEST(CanLoadSingleEntry) {
		AssertCanLoadSingleEntry<TTraits, TSerializer>(version);
	}
}}
