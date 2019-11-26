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

	TEST(TEST_CLASS, CanCreateExchangeEntry) {
		// Act:
		auto owner = test::GenerateRandomByteArray<Key>();
		auto entry = ExchangeEntry(owner);

		// Assert:
		EXPECT_EQ(owner, entry.owner());
	}

	TEST(TEST_CLASS, CanAccessBuyOffers) {
		// Arrange:
		auto entry = ExchangeEntry(Key());
		EXPECT_EQ(entry.buyOffers().end(), entry.buyOffers().find(MosaicId(1)));
		auto offer = state::BuyOffer{test::GenerateOffer(), test::GenerateRandomValue<Amount>()};

		// Act:
		entry.buyOffers().emplace(MosaicId(1), offer);

		// Assert:
		test::AssertOffer(offer, entry.buyOffers().find(MosaicId(1))->second);
	}

	TEST(TEST_CLASS, CanAccessSellOffers) {
		// Arrange:
		auto entry = ExchangeEntry(Key());
		EXPECT_EQ(entry.sellOffers().end(), entry.sellOffers().find(MosaicId(1)));
		auto offer = state::SellOffer{test::GenerateOffer()};

		// Act:
		entry.sellOffers().emplace(MosaicId(1), offer);

		// Assert:
		test::AssertOffer(offer, entry.sellOffers().find(MosaicId(1))->second);
	}
}}
