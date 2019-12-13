/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/state/ExchangeEntrySerializer.h"
#include "tests/test/core/SerializerTestUtils.h"
#include "tests/test/ExchangeTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace state {

#define TEST_CLASS ExchangeEntrySerializerTests

	namespace {
		constexpr size_t Offer_Count = 2;
		constexpr size_t Expired_Offer_Count = 2;
		constexpr auto Entry_Size =
			sizeof(VersionType)
			+ Key_Size
			+ 1 + Offer_Count * (sizeof(MosaicId) + sizeof(OfferBase))
			+ 1 + Offer_Count * (sizeof(MosaicId) + sizeof(OfferBase) + sizeof(Amount))
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

		void AssertEntryBuffer(const ExchangeEntry& entry, const uint8_t* pData, size_t expectedSize, VersionType version) {
			const auto* pExpectedEnd = pData + expectedSize;

			EXPECT_EQ(version, *reinterpret_cast<const VersionType*>(pData));
			pData += sizeof(VersionType);
			auto owner = Extract<Key_Size>(pData);
			EXPECT_EQ(entry.owner(), owner);
			pData += Key_Size;

			AssertSellOffers(entry.sellOffers(), pData);
			AssertBuyOffers(entry.buyOffers(), pData);

			AssertExpiredSellOffers(entry.expiredSellOffers(), pData);
			AssertExpiredBuyOffers(entry.expiredBuyOffers(), pData);

			EXPECT_EQ(pExpectedEnd, pData);
		}

		void AssertCanSaveSingleEntry(VersionType version) {
			// Arrange:
			TestContext context;
			auto entry = test::CreateExchangeEntry(Offer_Count, Expired_Offer_Count);

			// Act:
			ExchangeEntrySerializer::Save(entry, context.outputStream());

			// Assert:
			ASSERT_EQ(Entry_Size, context.buffer().size());
			AssertEntryBuffer(entry, context.buffer().data(), Entry_Size, version);
		}

		void AssertCanSaveMultipleEntries(VersionType version) {
			// Arrange:
			TestContext context;
			auto entry1 = test::CreateExchangeEntry(Offer_Count, Expired_Offer_Count);
			auto entry2 = test::CreateExchangeEntry(Offer_Count, Expired_Offer_Count);

			// Act:
			ExchangeEntrySerializer::Save(entry1, context.outputStream());
			ExchangeEntrySerializer::Save(entry2, context.outputStream());

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

		std::vector<uint8_t> CreateEntryBuffer(const ExchangeEntry& entry, VersionType version) {
			std::vector<uint8_t> buffer(Entry_Size);

			auto* pData = buffer.data();
			memcpy(pData, &version, sizeof(VersionType));
			pData += sizeof(VersionType);
			memcpy(pData, entry.owner().data(), Key_Size);
			pData += Key_Size;

			WriteSellOffers(entry.sellOffers(), pData);
			WriteBuyOffers(entry.buyOffers(), pData);

			WriteExpiredSellOffers(entry.expiredSellOffers(), pData);
			WriteExpiredBuyOffers(entry.expiredBuyOffers(), pData);

			return buffer;
		}

		void AssertCanLoadSingleEntry(VersionType version) {
			// Arrange:
			TestContext context;
			auto originalEntry = test::CreateExchangeEntry(Offer_Count, Expired_Offer_Count);
			auto buffer = CreateEntryBuffer(originalEntry, version);

			// Act:
			ExchangeEntry result((Key()));
			test::RunLoadValueTest<ExchangeEntrySerializer>(buffer, result);

			// Assert:
			test::AssertEqualExchangeData(originalEntry, result);
		}
	}

	TEST(TEST_CLASS, CanLoadSingleEntry_v1) {
		AssertCanLoadSingleEntry(1);
	}

	// endregion
}}
