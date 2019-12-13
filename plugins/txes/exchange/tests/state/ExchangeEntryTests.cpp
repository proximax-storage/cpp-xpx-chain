/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/state/ExchangeEntry.h"
#include "tests/test/ExchangeTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace state {

#define TEST_CLASS ExchangeEntryTests

	namespace {
		struct SellOfferTraits {
			static constexpr auto OfferType = model::OfferType::Sell;

			static model::OfferType InvalidOfferType() {
				return model::OfferType::Buy;
			}

			static SellOfferMap& GetOffers(ExchangeEntry& entry) {
				return entry.sellOffers();
			}

			static ExpiredSellOfferMap& GetExpiredOffers(ExchangeEntry& entry) {
				return entry.expiredSellOffers();
			}

			static SellOffer GenerateOffer() {
				return SellOffer{test::GenerateOffer()};
			}

			static SellOffer CreateOffer(const OfferBase& offer, const Amount&) {
				return SellOffer{ offer };
			}

			static void AssertResidualCost(const SellOffer&, const Amount&) {
			}
		};

		struct BuyOfferTraits {
			static constexpr auto OfferType = model::OfferType::Buy;

			static model::OfferType InvalidOfferType() {
				return model::OfferType::Sell;
			}

			static BuyOfferMap& GetOffers(ExchangeEntry& entry) {
				return entry.buyOffers();
			}

			static ExpiredBuyOfferMap& GetExpiredOffers(ExchangeEntry& entry) {
				return entry.expiredBuyOffers();
			}

			static BuyOffer GenerateOffer() {
				return BuyOffer{test::GenerateOffer(), test::GenerateRandomValue<Amount>()};
			}

			static BuyOffer CreateOffer(const OfferBase& offer, const Amount& residualCost) {
				return BuyOffer{ offer, residualCost };
			}

			static void AssertResidualCost(const BuyOffer& offer, const Amount& expected) {
				EXPECT_EQ(expected, offer.ResidualCost);
			}
		};
	}

#define TRAITS_BASED_TEST(TEST_NAME) \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(); \
	TEST(TEST_CLASS, TEST_NAME##_Sell) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<SellOfferTraits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_Buy) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<BuyOfferTraits>(); } \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)()

	TEST(TEST_CLASS, CannotGetOfferPriceWhenInitialAmountZero) {
		// Act:
		OfferBase offer{Amount(10), Amount(0), Amount(100), Height(1)};

		// Act + Assert:
		EXPECT_THROW(offer.price(), catapult_runtime_error);
	}

	TEST(TEST_CLASS, CanGetOfferBaseWhenInitialAmountNotZero) {
		// Act:
		OfferBase offer{Amount(10), Amount(10), Amount(100), Height(1)};

		// Act:
		auto price = static_cast<uint64_t>(offer.price());

		// Assert:
		EXPECT_EQ(10, price);
	}

	TRAITS_BASED_TEST(CannotGetOfferCostWhenInitialAmountZero) {
		// Act:
		auto offer = TTraits::CreateOffer({Amount(10), Amount(0), Amount(100), Height(1) }, Amount(100));

		// Act + Assert:
		EXPECT_THROW(offer.cost(Amount(10)), catapult_runtime_error);
	}

	TRAITS_BASED_TEST(CanGetOfferCostWhenInitialAmountNotZero) {
		// Act:
		auto offer = TTraits::CreateOffer({Amount(10), Amount(10), Amount(99), Height(1) }, Amount(100));

		// Act:
		auto cost = offer.cost(Amount(5));

		// Assert:
		EXPECT_EQ(Amount(TTraits::OfferType == model::OfferType::Sell ? 50 : 49), cost);
	}

	TRAITS_BASED_TEST(CannotAddAndSubtractMatchedOfferOfInvalidType) {
		// Act:
		auto offer = TTraits::CreateOffer({Amount(10), Amount(10), Amount(100), Height(1) }, Amount(100));
		model::MatchedOffer matchedOffer{ { { UnresolvedMosaicId(), Amount(1) }, Amount(10), TTraits::InvalidOfferType() }, Key()};

		// Act + Assert:
		EXPECT_THROW(offer += matchedOffer, catapult_invalid_argument);
		EXPECT_THROW(offer -= matchedOffer, catapult_invalid_argument);
	}

	TRAITS_BASED_TEST(CanAddMatchedOffer) {
		// Act:
		auto offer = TTraits::CreateOffer({Amount(10), Amount(10), Amount(100), Height(1) }, Amount(100));
		model::MatchedOffer matchedOffer{ { { UnresolvedMosaicId(), Amount(1) }, Amount(10), TTraits::OfferType }, Key()};

		// Act:
		offer += matchedOffer;

		// Assert:
		EXPECT_EQ(Amount(11), offer.Amount);
		TTraits::AssertResidualCost(offer, Amount(110));
	}

	TRAITS_BASED_TEST(CannotSubtractMatchedOfferWhenAmountIsGreater) {
		// Act:
		auto offer = TTraits::CreateOffer({Amount(10), Amount(10), Amount(100), Height(1) }, Amount(100));
		model::MatchedOffer matchedOffer{ { { UnresolvedMosaicId(), Amount(20) }, Amount(10), TTraits::OfferType }, Key()};

		// Act + Assert:
		EXPECT_THROW(offer -= matchedOffer, catapult_invalid_argument);
	}

	TEST(TEST_CLASS, CannotSubtractMatchedOfferWhenResidualCostIsGreater) {
		// Act:
		BuyOffer offer{ OfferBase{ Amount(10), Amount(10), Amount(100), Height(1) }, Amount(100) };
		model::MatchedOffer matchedOffer{ { { UnresolvedMosaicId(), Amount(5) }, Amount(200), model::OfferType::Buy }, Key()};

		// Act + Assert:
		EXPECT_THROW(offer -= matchedOffer, catapult_invalid_argument);
	}

	TRAITS_BASED_TEST(CanSubtractMatchedOfferWhenAmountIsNotGreater) {
		// Act:
		auto offer = TTraits::CreateOffer({Amount(10), Amount(10), Amount(100), Height(1) }, Amount(100));
		model::MatchedOffer matchedOffer{ { { UnresolvedMosaicId(), Amount(4) }, Amount(40), TTraits::OfferType }, Key()};

		// Act:
		offer -= matchedOffer;

		// Assert:
		EXPECT_EQ(Amount(6), offer.Amount);
		TTraits::AssertResidualCost(offer, Amount(60));
	}

	TEST(TEST_CLASS, CanCreateExchangeEntry) {
		// Act:
		auto owner = test::GenerateRandomByteArray<Key>();
		auto entry = ExchangeEntry(owner);

		// Assert:
		EXPECT_EQ(owner, entry.owner());
	}

	TRAITS_BASED_TEST(CanAccessOffers) {
		// Arrange:
		auto entry = ExchangeEntry(Key());
		auto& offers = TTraits::GetOffers(entry);
		ASSERT_TRUE(offers.empty());
		auto offer = TTraits::GenerateOffer();

		// Act:
		offers.emplace(MosaicId(1), offer);

		// Assert:
		offers = TTraits::GetOffers(entry);
		ASSERT_EQ(1, offers.size());
		test::AssertOffer(offer, offers.at(MosaicId(1)));
	}

	TRAITS_BASED_TEST(CanAccessExpiredOffers) {
		// Arrange:
		auto entry = ExchangeEntry(Key());
		auto& expiredOffers = TTraits::GetExpiredOffers(entry);
		ASSERT_TRUE(expiredOffers.empty());
		auto offer = TTraits::GenerateOffer();

		// Act:
		expiredOffers[Height(1)].emplace(MosaicId(1), offer);

		// Assert:
		expiredOffers = TTraits::GetExpiredOffers(entry);
		ASSERT_EQ(1, expiredOffers.size());
		const auto& offers = expiredOffers.at(Height(1));
		ASSERT_EQ(1, offers.size());
		test::AssertOffer(offer, offers.at(MosaicId(1)));
	}

	TEST(TEST_CLASS, MinExpiryHeight) {
		// Arrange:
		auto entry = ExchangeEntry(Key());
		ASSERT_EQ(ExchangeEntry::Invalid_Expiry_Height, entry.minExpiryHeight());
		entry.buyOffers().emplace(MosaicId(1), BuyOffer{ { Amount(10), Amount(10), Amount(100), Height(4) }, Amount(100) });
		entry.sellOffers().emplace(MosaicId(2), SellOffer{ { Amount(20), Amount(20), Amount(200), Height(3) } });
		entry.buyOffers().emplace(MosaicId(3), BuyOffer{ { Amount(30), Amount(30), Amount(300), Height(2) }, Amount(300) });
		entry.sellOffers().emplace(MosaicId(4), SellOffer{ { Amount(40), Amount(40), Amount(400), Height(1) } });

		// Act:
		auto minExpiryHeight = entry.minExpiryHeight();

		// Assert:
		EXPECT_EQ(Height(1), minExpiryHeight);
	}

	TEST(TEST_CLASS, MinPruneHeight) {
		// Arrange:
		auto entry = ExchangeEntry(Key());
		ASSERT_EQ(ExchangeEntry::Invalid_Expiry_Height, entry.minPruneHeight());
		entry.expiredBuyOffers()[Height(4)].emplace(MosaicId(1), BuyOffer{ { Amount(10), Amount(10), Amount(100), Height(4) }, Amount(100) });
		entry.expiredSellOffers()[Height(3)].emplace(MosaicId(2), SellOffer{ { Amount(20), Amount(20), Amount(200), Height(3) } });
		entry.expiredBuyOffers()[Height(2)].emplace(MosaicId(3), BuyOffer{ { Amount(30), Amount(30), Amount(300), Height(2) }, Amount(300) });
		entry.expiredSellOffers()[Height(1)].emplace(MosaicId(4), SellOffer{ { Amount(40), Amount(40), Amount(400), Height(1) } });

		// Act:
		auto minPruneHeight = entry.minPruneHeight();

		// Assert:
		EXPECT_EQ(Height(1), minPruneHeight);
	}

	TRAITS_BASED_TEST(CannotExpireSingleOfferWhenExpiredOfferExistsAtHeight) {
		// Arrange:
		auto entry = ExchangeEntry(Key());
		auto offer = TTraits::GenerateOffer();
		TTraits::GetOffers(entry).emplace(MosaicId(1), offer);
		TTraits::GetExpiredOffers(entry)[Height(10)].emplace(MosaicId(1), offer);

		// Act + Assert:
		EXPECT_THROW(entry.expireOffer(TTraits::OfferType, MosaicId(1), Height(10)), catapult_runtime_error);
	}

	TRAITS_BASED_TEST(CanExpireSingleOffer) {
		// Arrange:
		auto entry = ExchangeEntry(Key());
		ASSERT_TRUE(TTraits::GetExpiredOffers(entry).empty());
		auto offer = TTraits::GenerateOffer();
		TTraits::GetOffers(entry).emplace(MosaicId(1), offer);

		// Act:
		entry.expireOffer(TTraits::OfferType, MosaicId(1), Height(10));

		// Assert:
		auto& expiredOffers = TTraits::GetExpiredOffers(entry);
		ASSERT_EQ(1, expiredOffers.size());
		const auto& offers = expiredOffers.at(Height(10));
		ASSERT_EQ(1, offers.size());
		test::AssertOffer(offer, offers.at(MosaicId(1)));
	}

	TRAITS_BASED_TEST(CannotUnxpireSingleOfferWhenOfferExistsAtHeight) {
		// Arrange:
		auto entry = ExchangeEntry(Key());
		auto offer = TTraits::GenerateOffer();
		TTraits::GetOffers(entry).emplace(MosaicId(1), offer);
		TTraits::GetExpiredOffers(entry)[Height(10)].emplace(MosaicId(1), offer);

		// Act + Assert:
		EXPECT_THROW(entry.unexpireOffer(TTraits::OfferType, MosaicId(1), Height(10)), catapult_runtime_error);
	}

	TRAITS_BASED_TEST(CanUnexpireSingleOffer) {
		// Arrange:
		auto entry = ExchangeEntry(Key());
		ASSERT_TRUE(TTraits::GetOffers(entry).empty());
		auto offer = TTraits::GenerateOffer();
		TTraits::GetExpiredOffers(entry)[Height(10)].emplace(MosaicId(1), offer);

		// Act:
		entry.unexpireOffer(TTraits::OfferType, MosaicId(1), Height(10));

		// Assert:
		auto& offers = TTraits::GetOffers(entry);
		ASSERT_EQ(1, offers.size());
		test::AssertOffer(offer, offers.at(MosaicId(1)));
		EXPECT_TRUE(TTraits::GetExpiredOffers(entry).empty());
	}

	TEST(TEST_CLASS, CannotExpireOffersWhenExpiredOfferExistsAtHeight) {
		// Arrange:
		bool expireBuyOffersCalled = false;
		bool expireSellOffersCalled = false;

		auto entry = ExchangeEntry(Key());
		entry.buyOffers().emplace(MosaicId(1), BuyOffer{ { Amount(10), Amount(10), Amount(100), Height(3) }, Amount(100) });
		entry.sellOffers().emplace(MosaicId(2), SellOffer{ { Amount(20), Amount(20), Amount(200), Height(2) } });
		entry.buyOffers().emplace(MosaicId(3), BuyOffer{ { Amount(30), Amount(30), Amount(300), Height(2) }, Amount(300) });
		entry.sellOffers().emplace(MosaicId(4), SellOffer{ { Amount(40), Amount(40), Amount(400), Height(1) } });
		entry.expiredSellOffers()[Height(2)].emplace(MosaicId(2), SellOffer{ { Amount(20), Amount(20), Amount(200), Height(2) } });

		// Act:
		EXPECT_THROW(
			entry.expireOffers(
				Height(2),
				[&expireBuyOffersCalled](const auto& ) {
					expireBuyOffersCalled = true;
				},
				[&expireSellOffersCalled](const auto& ) {
					expireSellOffersCalled = true;
				}),
			catapult_runtime_error
		);

		// Assert:
		EXPECT_TRUE(expireBuyOffersCalled);
		EXPECT_FALSE(expireSellOffersCalled);
	}

	TEST(TEST_CLASS, CanExpireOffers) {
		// Arrange:
		bool expireBuyOffersCalled = false;
		bool expireSellOffersCalled = false;

		auto entry = ExchangeEntry(Key());
		entry.buyOffers().emplace(MosaicId(1), BuyOffer{ { Amount(10), Amount(10), Amount(100), Height(3) }, Amount(100) });
		entry.sellOffers().emplace(MosaicId(2), SellOffer{ { Amount(20), Amount(20), Amount(200), Height(2) } });
		entry.buyOffers().emplace(MosaicId(3), BuyOffer{ { Amount(30), Amount(30), Amount(300), Height(2) }, Amount(300) });
		entry.sellOffers().emplace(MosaicId(4), SellOffer{ { Amount(40), Amount(40), Amount(400), Height(1) } });

		auto expectedEntry = ExchangeEntry(Key());
		expectedEntry.buyOffers().emplace(MosaicId(1), BuyOffer{ { Amount(10), Amount(10), Amount(100), Height(3) }, Amount(100) });
		expectedEntry.sellOffers().emplace(MosaicId(4), SellOffer{ { Amount(40), Amount(40), Amount(400), Height(1) } });
		expectedEntry.expiredSellOffers()[Height(2)].emplace(MosaicId(2), SellOffer{ { Amount(20), Amount(20), Amount(200), Height(2) } });
		expectedEntry.expiredBuyOffers()[Height(2)].emplace(MosaicId(3), BuyOffer{ { Amount(30), Amount(30), Amount(300), Height(2) }, Amount(300) });

		// Act:
		entry.expireOffers(
			Height(2),
			[&expireBuyOffersCalled](const auto& ) {
				expireBuyOffersCalled = true;
			},
			[&expireSellOffersCalled](const auto& ) {
				expireSellOffersCalled = true;
			}
		);

		// Assert:
		test::AssertEqualExchangeData(expectedEntry, entry);
		EXPECT_TRUE(expireBuyOffersCalled);
		EXPECT_TRUE(expireSellOffersCalled);
	}

	TEST(TEST_CLASS, CannotUnexpireOffersWhenOfferExistsAtHeight) {
		// Arrange:
		bool unexpireBuyOffersCalled = false;
		bool unexpireSellOffersCalled = false;

		auto entry = ExchangeEntry(Key());
		entry.buyOffers().emplace(MosaicId(1), BuyOffer{ { Amount(10), Amount(10), Amount(100), Height(3) }, Amount(100) });
		entry.sellOffers().emplace(MosaicId(4), SellOffer{ { Amount(40), Amount(40), Amount(400), Height(1) } });
		entry.sellOffers().emplace(MosaicId(2), SellOffer{ { Amount(20), Amount(20), Amount(200), Height(2) } });
		entry.expiredSellOffers()[Height(2)].emplace(MosaicId(2), SellOffer{ { Amount(20), Amount(20), Amount(200), Height(2) } });
		entry.expiredBuyOffers()[Height(2)].emplace(MosaicId(3), BuyOffer{ { Amount(30), Amount(30), Amount(300), Height(2) }, Amount(300) });

		// Act:
		EXPECT_THROW(
			entry.unexpireOffers(
				Height(2),
				[&unexpireBuyOffersCalled](const auto& ) {
					unexpireBuyOffersCalled = true;
				},
				[&unexpireSellOffersCalled](const auto& ) {
					unexpireSellOffersCalled = true;
				}),
			catapult_runtime_error
		);

		// Assert:
		EXPECT_TRUE(unexpireBuyOffersCalled);
		EXPECT_FALSE(unexpireSellOffersCalled);
	}

	TEST(TEST_CLASS, CanUnexpireOffers) {
		// Arrange:
		bool unexpireBuyOffersCalled = false;
		bool unexpireSellOffersCalled = false;

		auto entry = ExchangeEntry(Key());
		entry.buyOffers().emplace(MosaicId(1), BuyOffer{ { Amount(10), Amount(10), Amount(100), Height(3) }, Amount(100) });
		entry.sellOffers().emplace(MosaicId(4), SellOffer{ { Amount(40), Amount(40), Amount(400), Height(1) } });
		entry.expiredSellOffers()[Height(2)].emplace(MosaicId(2), SellOffer{ { Amount(20), Amount(20), Amount(200), Height(2) } });
		entry.expiredBuyOffers()[Height(2)].emplace(MosaicId(3), BuyOffer{ { Amount(30), Amount(30), Amount(300), Height(2) }, Amount(300) });

		auto expectedEntry = ExchangeEntry(Key());
		expectedEntry.buyOffers().emplace(MosaicId(1), BuyOffer{ { Amount(10), Amount(10), Amount(100), Height(3) }, Amount(100) });
		expectedEntry.sellOffers().emplace(MosaicId(2), SellOffer{ { Amount(20), Amount(20), Amount(200), Height(2) } });
		expectedEntry.buyOffers().emplace(MosaicId(3), BuyOffer{ { Amount(30), Amount(30), Amount(300), Height(2) }, Amount(300) });
		expectedEntry.sellOffers().emplace(MosaicId(4), SellOffer{ { Amount(40), Amount(40), Amount(400), Height(1) } });

		// Act:
		entry.unexpireOffers(
			Height(2),
			[&unexpireBuyOffersCalled](const auto& ) {
				unexpireBuyOffersCalled = true;
			},
			[&unexpireSellOffersCalled](const auto& ) {
				unexpireSellOffersCalled = true;
			}
		);

		// Assert:
		test::AssertEqualExchangeData(expectedEntry, entry);
		EXPECT_TRUE(unexpireBuyOffersCalled);
		EXPECT_TRUE(unexpireSellOffersCalled);
	}

	TEST(TEST_CLASS, OfferExists) {
		// Arrange:
		auto entry = ExchangeEntry(Key());
		entry.buyOffers().emplace(MosaicId(1), BuyOffer{ { Amount(10), Amount(10), Amount(100), Height(3) }, Amount(100) });
		entry.sellOffers().emplace(MosaicId(2), SellOffer{ { Amount(20), Amount(20), Amount(200), Height(2) } });
		entry.buyOffers().emplace(MosaicId(3), BuyOffer{ { Amount(30), Amount(30), Amount(300), Height(2) }, Amount(300) });
		entry.sellOffers().emplace(MosaicId(4), SellOffer{ { Amount(40), Amount(40), Amount(400), Height(1) } });

		// Assert:
		EXPECT_TRUE(entry.offerExists(model::OfferType::Buy, MosaicId(1)));
		EXPECT_FALSE(entry.offerExists(model::OfferType::Sell, MosaicId(1)));
		EXPECT_FALSE(entry.offerExists(model::OfferType::Buy, MosaicId(2)));
		EXPECT_TRUE(entry.offerExists(model::OfferType::Sell, MosaicId(2)));
		EXPECT_TRUE(entry.offerExists(model::OfferType::Buy, MosaicId(3)));
		EXPECT_FALSE(entry.offerExists(model::OfferType::Sell, MosaicId(3)));
		EXPECT_FALSE(entry.offerExists(model::OfferType::Buy, MosaicId(4)));
		EXPECT_TRUE(entry.offerExists(model::OfferType::Sell, MosaicId(4)));
		EXPECT_FALSE(entry.offerExists(model::OfferType::Buy, MosaicId(5)));
		EXPECT_FALSE(entry.offerExists(model::OfferType::Sell, MosaicId(5)));
	}

	TEST(TEST_CLASS, AddOffer) {
		// Arrange:
		auto entry = ExchangeEntry(Key());
		model::OfferWithDuration buyOffer{ { { UnresolvedMosaicId(1), Amount(10) }, Amount(100), model::OfferType::Buy }, BlockDuration(100)};
		model::OfferWithDuration sellOffer{ { { UnresolvedMosaicId(2), Amount(20) }, Amount(200), model::OfferType::Sell }, BlockDuration(100)};

		auto expectedEntry = ExchangeEntry(Key());
		expectedEntry.buyOffers().emplace(MosaicId(1), BuyOffer{ { Amount(10), Amount(10), Amount(100), Height(1) }, Amount(100) });
		expectedEntry.sellOffers().emplace(MosaicId(2), SellOffer{ { Amount(20), Amount(20), Amount(200), Height(2) } });

		// Act:
		entry.addOffer(MosaicId(1), &buyOffer, Height(1));
		entry.addOffer(MosaicId(2), &sellOffer, Height(2));

		// Assert:
		test::AssertEqualExchangeData(expectedEntry, entry);
	}

	TEST(TEST_CLASS, RemoveOffer) {
		// Arrange:
		auto entry = ExchangeEntry(Key());
		entry.buyOffers().emplace(MosaicId(1), BuyOffer{ { Amount(10), Amount(10), Amount(100), Height(1) }, Amount(100) });
		entry.sellOffers().emplace(MosaicId(2), SellOffer{ { Amount(20), Amount(20), Amount(200), Height(2) } });
		entry.buyOffers().emplace(MosaicId(3), BuyOffer{ { Amount(30), Amount(30), Amount(300), Height(3) }, Amount(300) });
		entry.sellOffers().emplace(MosaicId(4), SellOffer{ { Amount(40), Amount(40), Amount(400), Height(4) } });

		auto expectedEntry = ExchangeEntry(Key());
		expectedEntry.buyOffers().emplace(MosaicId(3), BuyOffer{ { Amount(30), Amount(30), Amount(300), Height(3) }, Amount(300) });
		expectedEntry.sellOffers().emplace(MosaicId(4), SellOffer{ { Amount(40), Amount(40), Amount(400), Height(4) } });

		// Act:
		entry.removeOffer(model::OfferType::Buy, MosaicId(1));
		entry.removeOffer(model::OfferType::Sell, MosaicId(2));
		entry.removeOffer(model::OfferType::Buy, MosaicId(5));
		entry.removeOffer(model::OfferType::Sell, MosaicId(5));

		// Assert:
		test::AssertEqualExchangeData(expectedEntry, entry);
	}

	TEST(TEST_CLASS, CannotGetBaseOfferWhenOfferDoesntExist) {
		// Arrange:
		auto entry = ExchangeEntry(Key());
		entry.buyOffers().emplace(MosaicId(2), BuyOffer{ test::GenerateOffer(), Amount(100) });
		entry.sellOffers().emplace(MosaicId(3), SellOffer{ test::GenerateOffer() });

		// Assert:
		EXPECT_THROW(entry.getBaseOffer(model::OfferType::Buy, MosaicId(1)), catapult_invalid_argument);
		EXPECT_THROW(entry.getBaseOffer(model::OfferType::Sell, MosaicId(1)), catapult_invalid_argument);
	}

	TEST(TEST_CLASS, CanGetBaseOffer) {
		// Arrange:
		auto entry = ExchangeEntry(Key());
		auto offer = test::GenerateOffer();
		entry.buyOffers().emplace(MosaicId(1), BuyOffer{ offer, Amount(100) });
		entry.sellOffers().emplace(MosaicId(2), SellOffer{ offer });

		// Assert:
		test::AssertOffer(offer, entry.getBaseOffer(model::OfferType::Buy, MosaicId(1)));
		test::AssertOffer(offer, entry.getBaseOffer(model::OfferType::Sell, MosaicId(2)));
	}

	TEST(TEST_CLASS, Empty) {
		// Arrange:
		auto entry = ExchangeEntry(Key());

		// Assert:
		EXPECT_TRUE(entry.empty());
		entry.buyOffers().emplace(MosaicId(1), BuyOffer{ test::GenerateOffer(), Amount(100) });
		EXPECT_FALSE(entry.empty());
		entry.buyOffers().erase(MosaicId(1));
		entry.sellOffers().emplace(MosaicId(2), SellOffer{ test::GenerateOffer() });
		EXPECT_FALSE(entry.empty());
		entry.sellOffers().erase(MosaicId(2));
		entry.expiredBuyOffers()[Height(2)].emplace(MosaicId(1), BuyOffer{ test::GenerateOffer(), Amount(100) });
		EXPECT_FALSE(entry.empty());
		entry.expiredBuyOffers().erase(Height(2));
		entry.expiredSellOffers()[Height(2)].emplace(MosaicId(2), SellOffer{ test::GenerateOffer() });
		EXPECT_FALSE(entry.empty());
	}
}}
