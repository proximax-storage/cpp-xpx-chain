/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ExchangeTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace test {

	state::OfferBase GenerateOffer() {
		return state::OfferBase{
			test::GenerateRandomValue<Amount>(),
			test::GenerateRandomValue<Amount>(),
			test::GenerateRandomValue<Amount>(),
			test::GenerateRandomValue<Height>(),
		};
	}

	state::ExchangeEntry CreateExchangeEntry(uint8_t offerCount, uint8_t expiredOfferCount, Key key, VersionType version) {
		state::ExchangeEntry entry(key, version);
		for (uint8_t i = 1; i <= offerCount; ++i) {
			entry.buyOffers().emplace(MosaicId(i), state::BuyOffer{test::GenerateOffer(), test::GenerateRandomValue<Amount>()});
			entry.sellOffers().emplace(MosaicId(i), state::SellOffer{test::GenerateOffer()});
		}
		for (uint8_t i = 1; i <= expiredOfferCount; ++i) {
			state::BuyOfferMap buyOffers;
			buyOffers.emplace(MosaicId(i), state::BuyOffer{test::GenerateOffer(), test::GenerateRandomValue<Amount>()});
			entry.expiredBuyOffers().emplace(Height(i), buyOffers);
			state::SellOfferMap sellOffers;
			sellOffers.emplace(MosaicId(i), state::SellOffer{test::GenerateOffer()});
			entry.expiredSellOffers().emplace(Height(i), sellOffers);
		}
		return entry;
	}

	void AssertOffer(const model::Offer& offer1, const model::Offer& offer2) {
		EXPECT_EQ(offer1.Mosaic.MosaicId, offer2.Mosaic.MosaicId);
		EXPECT_EQ(offer1.Mosaic.Amount, offer2.Mosaic.Amount);
		EXPECT_EQ(offer1.Cost, offer2.Cost);
		EXPECT_EQ(offer1.Type, offer2.Type);
	}

	void AssertOffer(const state::OfferBase& offer1, const state::OfferBase& offer2) {
		EXPECT_EQ(offer1.Amount, offer2.Amount);
		EXPECT_EQ(offer1.InitialAmount, offer2.InitialAmount);
		EXPECT_EQ(offer1.InitialCost, offer2.InitialCost);
		EXPECT_EQ(offer1.Deadline, offer2.Deadline);
	}

	void AssertOffer(const state::BuyOffer& offer1, const state::BuyOffer& offer2) {
		AssertOffer(dynamic_cast<const state::OfferBase&>(offer1), dynamic_cast<const state::OfferBase&>(offer2));
		EXPECT_EQ(offer1.ResidualCost, offer2.ResidualCost);
	}

	void AssertOffer(const state::SellOffer& offer1, const state::SellOffer& offer2) {
		AssertOffer(dynamic_cast<const state::OfferBase&>(offer1), dynamic_cast<const state::OfferBase&>(offer2));
	}

	namespace {
		template<typename TOfferMap>
		void AssertOffers(const TOfferMap& offers1, const TOfferMap& offers2) {
			ASSERT_EQ(offers1.size(), offers2.size());
			for (const auto& pair : offers1) {
				AssertOffer(pair.second, offers2.find(pair.first)->second);
			}
		}

		template<typename TExpiredOfferMap>
		void AssertExpiredOffers(const TExpiredOfferMap& offers1, const TExpiredOfferMap& offers2) {
			ASSERT_EQ(offers1.size(), offers2.size());
			for (const auto& pair : offers1) {
				AssertOffers(pair.second, offers2.find(pair.first)->second);
			}
		}
	}

	void AssertEqualExchangeData(const state::ExchangeEntry& entry1, const state::ExchangeEntry& entry2) {
		EXPECT_EQ(entry1.version(), entry2.version());
		EXPECT_EQ(entry1.owner(), entry2.owner());
		AssertOffers(entry1.buyOffers(), entry2.buyOffers());
		AssertOffers(entry1.sellOffers(), entry2.sellOffers());
		AssertExpiredOffers(entry1.expiredBuyOffers(), entry2.expiredBuyOffers());
		AssertExpiredOffers(entry1.expiredSellOffers(), entry2.expiredSellOffers());
	}
}}


