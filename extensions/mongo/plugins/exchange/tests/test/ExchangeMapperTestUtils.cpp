/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ExchangeMapperTestUtils.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/tests/test/MapperTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace test {

	namespace {
		void AssertOffer(const state::OfferBase& offer, const bsoncxx::document::element& dbOffer) {
			EXPECT_EQ(offer.Amount.unwrap(), GetUint64(dbOffer, "amount"));
			EXPECT_EQ(offer.InitialAmount.unwrap(), GetUint64(dbOffer, "initialAmount"));
			EXPECT_EQ(offer.InitialCost.unwrap(), GetUint64(dbOffer, "initialCost"));
			EXPECT_EQ(offer.Deadline.unwrap(), GetUint64(dbOffer, "deadline"));
			EXPECT_EQ(offer.price(), dbOffer["price"].get_double());
		}

		void AssertBuyOffers(const state::BuyOfferMap& offers, const bsoncxx::document::view& dbOffers) {
			ASSERT_EQ(offers.size(), test::GetFieldCount(dbOffers));

			for (const auto& dbOffer : dbOffers) {
				auto mosaicId = GetMosaicId(dbOffer, "mosaicId");
				auto& offer = offers.at(mosaicId);
				AssertOffer(offer, dbOffer);
				EXPECT_EQ(offer.ResidualCost.unwrap(), GetUint64(dbOffer, "residualCost"));
			}
		}

		void AssertSellOffers(const state::SellOfferMap& offers, const bsoncxx::document::view& dbOffers) {
			ASSERT_EQ(offers.size(), test::GetFieldCount(dbOffers));

			for (const auto& dbOffer : dbOffers) {
				auto mosaicId = GetMosaicId(dbOffer, "mosaicId");
				auto& offer = offers.at(mosaicId);
				AssertOffer(offer, dbOffer);
			}
		}

		void AssertExpiredBuyOffers(const state::ExpiredBuyOfferMap& expiredOffers, const bsoncxx::document::view& dbOffers) {
			ASSERT_EQ(expiredOffers.size(), test::GetFieldCount(dbOffers));

			for (const auto& dbOffer : dbOffers) {
				auto height = Height{GetUint64(dbOffer, "height")};
				auto& offers = expiredOffers.at(height);
				AssertBuyOffers(offers, dbOffer["buyOffers"].get_array().value);
			}
		}

		void AssertExpiredSellOffers(const state::ExpiredSellOfferMap& expiredOffers, const bsoncxx::document::view& dbOffers) {
			ASSERT_EQ(expiredOffers.size(), test::GetFieldCount(dbOffers));

			for (const auto& dbOffer : dbOffers) {
				auto height = Height{GetUint64(dbOffer, "height")};
				auto& offers = expiredOffers.at(height);
				AssertSellOffers(offers, dbOffer["sellOffers"].get_array().value);
			}
		}
	}

	void AssertEqualExchangeData(const state::ExchangeEntry& entry, const Address& address, const bsoncxx::document::view& dbExchangeEntry) {
		EXPECT_EQ(6u, test::GetFieldCount(dbExchangeEntry));

		EXPECT_EQ(entry.owner(), GetKeyValue(dbExchangeEntry, "owner"));
		EXPECT_EQ(address, test::GetAddressValue(dbContract, "ownerAddress"));

		AssertBuyOffers(entry.buyOffers(), dbExchangeEntry["buyOffers"].get_array().value);
		AssertSellOffers(entry.sellOffers(), dbExchangeEntry["sellOffers"].get_array().value);

		AssertExpiredBuyOffers(entry.expiredBuyOffers(), dbExchangeEntry["expiredBuyOffers"].get_array().value);
		AssertExpiredSellOffers(entry.expiredSellOffers(), dbExchangeEntry["expiredSellOffers"].get_array().value);
	}
}}
