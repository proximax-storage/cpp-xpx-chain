/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/state/SdaExchangeEntry.h"
#include "tests/test/SdaExchangeTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace state {

#define TEST_CLASS SdaExchangeEntryTests

	namespace {
		struct SdaOfferBalanceTraits {
			static SdaOfferBalanceMap& GetSdaOfferBalances(SdaExchangeEntry& entry) {
				return entry.sdaOfferBalances();
			}

			static ExpiredSdaOfferBalanceMap& GetExpiredSdaOfferBalances(SdaExchangeEntry& entry) {
				return entry.expiredSdaOfferBalances();
			}

			static SdaOfferBalance GenerateSdaOfferBalance() {
				return SdaOfferBalance{test::GenerateSdaOfferBalance()};
			}

			static SdaOfferBalance CreateSdaOfferBalance(const SdaOfferBalance& offer) {
				return SdaOfferBalance{ offer };
			}
		};
	}

#define TRAITS_BASED_TEST(TEST_NAME) \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(); \
	TEST(TEST_CLASS, TEST_NAME##_SdaOfferBalance) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<SdaOfferBalanceTraits>(); } \
    template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)()

	TEST(TEST_CLASS, CanCreateSdaExchangeEntry) {
		// Act:
		auto owner = test::GenerateRandomByteArray<Key>();
		auto entry = SdaExchangeEntry(owner);

		// Assert:
		EXPECT_EQ(owner, entry.owner());
		EXPECT_EQ(1, entry.version());
	}

	TRAITS_BASED_TEST(CanAccessSdaOffers) {
		// Arrange:
		auto entry = SdaExchangeEntry(Key());
		auto& offers = TTraits::GetSdaOfferBalances(entry);
		ASSERT_TRUE(offers.empty());
		auto offer = TTraits::GenerateSdaOfferBalance();

		// Act:
        MosaicsPair pair{MosaicId(1), MosaicId(1)};
		offers.emplace(pair, offer);

		// Assert:
		offers = TTraits::GetSdaOfferBalances(entry);
		ASSERT_EQ(1, offers.size());
		test::AssertSdaOfferBalance(offer, offers.at(pair));
	}

	TRAITS_BASED_TEST(CanAccessExpiredSdaOffers) {
		// Arrange:
		auto entry = SdaExchangeEntry(Key());
		auto& expiredOffers = TTraits::GetExpiredSdaOfferBalances(entry);
		ASSERT_TRUE(expiredOffers.empty());
		auto offer = TTraits::GenerateSdaOfferBalance();

		// Act:
        MosaicsPair pair{MosaicId(1), MosaicId(1)};
		expiredOffers[Height(1)].emplace(pair, offer);

		// Assert:
		expiredOffers = TTraits::GetExpiredSdaOfferBalances(entry);
		ASSERT_EQ(1, expiredOffers.size());
		const auto& offers = expiredOffers.at(Height(1));
		ASSERT_EQ(1, offers.size());
		test::AssertSdaOfferBalance(offer, offers.at(pair));
	}

	TEST(TEST_CLASS, MinExpiryHeight) {
		// Arrange:
		auto entry = SdaExchangeEntry(Key());
		ASSERT_EQ(SdaExchangeEntry::Invalid_Expiry_Height, entry.minExpiryHeight());

        MosaicsPair pair1{MosaicId(1), MosaicId(1)};
        MosaicsPair pair2{MosaicId(2), MosaicId(2)};
		entry.sdaOfferBalances().emplace(pair1, SdaOfferBalance{ Amount(100), Amount(200), Amount(100), Amount(200), Height(4) });
		entry.sdaOfferBalances().emplace(pair2, SdaOfferBalance{ Amount(200), Amount(300), Amount(200), Amount(300), Height(3) });

		// Act:
		auto minExpiryHeight = entry.minExpiryHeight();

		// Assert:
		EXPECT_EQ(Height(3), minExpiryHeight);
	}

	TEST(TEST_CLASS, MinPruneHeight) {
		// Arrange:
		auto entry = SdaExchangeEntry(Key());
		ASSERT_EQ(SdaExchangeEntry::Invalid_Expiry_Height, entry.minPruneHeight());
        MosaicsPair pair1{MosaicId(1), MosaicId(1)};
        MosaicsPair pair2{MosaicId(2), MosaicId(2)};
		entry.expiredSdaOfferBalances()[Height(4)].emplace(pair1, SdaOfferBalance{ Amount(100), Amount(200), Amount(100), Amount(200), Height(4) });
		entry.expiredSdaOfferBalances()[Height(3)].emplace(pair2, SdaOfferBalance{ Amount(200), Amount(300), Amount(200), Amount(300), Height(3) });

		// Act:
		auto minPruneHeight = entry.minPruneHeight();

		// Assert:
		EXPECT_EQ(Height(3), minPruneHeight);
	}

	TRAITS_BASED_TEST(CannotExpireSingleSdaOfferWhenExpiredSdaOfferExistsAtHeight) {
		// Arrange:
		auto entry = SdaExchangeEntry(Key());
		auto offer = TTraits::GenerateSdaOfferBalance();
        MosaicsPair pair{MosaicId(1), MosaicId(1)};
		TTraits::GetSdaOfferBalances(entry).emplace(pair, offer);
		TTraits::GetExpiredSdaOfferBalances(entry)[Height(10)].emplace(pair, offer);

		// Act + Assert:
		EXPECT_THROW(entry.expireOffer(pair, Height(10)), catapult_runtime_error);
	}

	TRAITS_BASED_TEST(CanExpireSingleSdaOffer) {
		// Arrange:
		auto entry = SdaExchangeEntry(Key());
		ASSERT_TRUE(TTraits::GetExpiredSdaOfferBalances(entry).empty());
		auto offer = TTraits::GenerateSdaOfferBalance();
        MosaicsPair pair{MosaicId(1), MosaicId(1)};
		TTraits::GetSdaOfferBalances(entry).emplace(pair, offer);

		// Act:
		entry.expireOffer(pair, Height(10));

		// Assert:
		auto& expiredOffers = TTraits::GetExpiredSdaOfferBalances(entry);
		ASSERT_EQ(1, expiredOffers.size());
		const auto& offers = expiredOffers.at(Height(10));
		ASSERT_EQ(1, offers.size());
		test::AssertSdaOfferBalance(offer, offers.at(pair));
	}

	TRAITS_BASED_TEST(CannotUnxpireSingleSdaOfferWhenSdaOfferExistsAtHeight) {
		// Arrange:
		auto entry = SdaExchangeEntry(Key());
		auto offer = TTraits::GenerateSdaOfferBalance();
        MosaicsPair pair{MosaicId(1), MosaicId(1)};
		TTraits::GetSdaOfferBalances(entry).emplace(pair, offer);
		TTraits::GetExpiredSdaOfferBalances(entry)[Height(10)].emplace(pair, offer);

		// Act + Assert:
		EXPECT_THROW(entry.unexpireOffer(pair, Height(10)), catapult_runtime_error);
	}

	TRAITS_BASED_TEST(CanUnexpireSingleSdaOffer) {
		// Arrange:
		auto entry = SdaExchangeEntry(Key());
		ASSERT_TRUE(TTraits::GetSdaOfferBalances(entry).empty());
		auto offer = TTraits::GenerateSdaOfferBalance();
        MosaicsPair pair{MosaicId(1), MosaicId(1)};
		TTraits::GetExpiredSdaOfferBalances(entry)[Height(10)].emplace(pair, offer);

		// Act:
		entry.unexpireOffer(pair, Height(10));

		// Assert:
		auto& offers = TTraits::GetSdaOfferBalances(entry);
		ASSERT_EQ(1, offers.size());
		test::AssertSdaOfferBalance(offer, offers.at(pair));
		EXPECT_TRUE(TTraits::GetExpiredSdaOfferBalances(entry).empty());
	}

	TEST(TEST_CLASS, CannotExpireSdaOffersWhenExpiredSdaOfferExistsAtHeight) {
		// Arrange:
		bool expireSdaOfferBalanceCalled = false;

		auto entry = SdaExchangeEntry(Key());
        MosaicsPair pair1{MosaicId(1), MosaicId(1)};
        MosaicsPair pair2{MosaicId(2), MosaicId(2)};
		entry.sdaOfferBalances().emplace(pair1, SdaOfferBalance{ Amount(100), Amount(10), Amount(100), Amount(10), Height(1) });
		entry.sdaOfferBalances().emplace(pair2, SdaOfferBalance{ Amount(200), Amount(20), Amount(200), Amount(20), Height(2) });
        entry.expiredSdaOfferBalances()[Height(2)].emplace(pair2, SdaOfferBalance{ Amount(200), Amount(20), Amount(200), Amount(20), Height(2) });

		// Act:    
		EXPECT_THROW(
            entry.expireOffers(Height(2), [&expireSdaOfferBalanceCalled](const auto& ) {
                expireSdaOfferBalanceCalled = true;
            }),
			catapult_runtime_error
		);

		// Assert:
		EXPECT_FALSE(expireSdaOfferBalanceCalled);
	}

	TEST(TEST_CLASS, CanExpireSdaOffers) {
		// Arrange:
		bool expireSdaOfferBalanceCalled = false;
        MosaicsPair pair1{MosaicId(1), MosaicId(1)};
        MosaicsPair pair2{MosaicId(2), MosaicId(2)};
        MosaicsPair pair3{MosaicId(3), MosaicId(3)};

		auto entry = SdaExchangeEntry(Key());
		entry.sdaOfferBalances().emplace(pair1, SdaOfferBalance{ Amount(100), Amount(10), Amount(100), Amount(10), Height(2) });
		entry.sdaOfferBalances().emplace(pair2, SdaOfferBalance{ Amount(200), Amount(20), Amount(200), Amount(20), Height(2) });
        entry.sdaOfferBalances().emplace(pair3, SdaOfferBalance{ Amount(300), Amount(30), Amount(300), Amount(30), Height(3) });

		auto expectedEntry = SdaExchangeEntry(Key());
		expectedEntry.expiredSdaOfferBalances()[Height(2)].emplace(pair1, SdaOfferBalance{ Amount(100), Amount(10), Amount(100), Amount(10), Height(2) });
		expectedEntry.expiredSdaOfferBalances()[Height(2)].emplace(pair2, SdaOfferBalance{ Amount(200), Amount(20), Amount(200), Amount(20), Height(2) });
        expectedEntry.sdaOfferBalances().emplace(pair3, SdaOfferBalance{ Amount(300), Amount(30), Amount(300), Amount(30), Height(3) });

		// Act:
		entry.expireOffers(Height(2), [&expireSdaOfferBalanceCalled](const auto& ) {
            expireSdaOfferBalanceCalled = true;
        });

		// Assert:
		test::AssertEqualSdaExchangeData(expectedEntry, entry);
		EXPECT_TRUE(expireSdaOfferBalanceCalled);
	}

	TEST(TEST_CLASS, CannotUnxpireSdaOffersWhenExpiredSdaOfferExistsAtHeight) {
		// Arrange:
		bool unexpireSdaOfferBalanceCalled = false;

        auto entry = SdaExchangeEntry(Key());
        MosaicsPair pair1{MosaicId(1), MosaicId(1)};
        MosaicsPair pair2{MosaicId(2), MosaicId(2)};
        MosaicsPair pair3{MosaicId(3), MosaicId(3)};
        entry.sdaOfferBalances().emplace(pair1, SdaOfferBalance{ Amount(100), Amount(10), Amount(100), Amount(10), Height(2) });
        entry.sdaOfferBalances().emplace(pair2, SdaOfferBalance{ Amount(200), Amount(20), Amount(200), Amount(20), Height(2) });
        entry.sdaOfferBalances().emplace(pair3, SdaOfferBalance{ Amount(300), Amount(30), Amount(300), Amount(30), Height(3) });
		entry.expiredSdaOfferBalances()[Height(2)].emplace(pair2,  SdaOfferBalance{ Amount(200), Amount(20), Amount(200), Amount(20), Height(2) });

		// Act:
		EXPECT_THROW(
			entry.unexpireOffers(Height(2), [&unexpireSdaOfferBalanceCalled](const auto& ) {
                unexpireSdaOfferBalanceCalled = true;
            }),
			catapult_runtime_error
		);

		// Assert:
		EXPECT_FALSE(unexpireSdaOfferBalanceCalled);
	}

	TEST(TEST_CLASS, CanUnexpireSdaOffers) {
		// Arrange:
		bool unexpireSdaOfferBalanceCalled = false;
        MosaicsPair pair1{MosaicId(1), MosaicId(1)};
        MosaicsPair pair2{MosaicId(2), MosaicId(2)};
        MosaicsPair pair3{MosaicId(3), MosaicId(3)};
		MosaicsPair pair4{MosaicId(4), MosaicId(4)};

        auto entry = SdaExchangeEntry(Key());
        entry.sdaOfferBalances().emplace(pair1, SdaOfferBalance{ Amount(100), Amount(10), Amount(100), Amount(10), Height(2) });
        entry.sdaOfferBalances().emplace(pair4, SdaOfferBalance{ Amount(300), Amount(30), Amount(300), Amount(30), Height(3) });
        entry.expiredSdaOfferBalances()[Height(2)].emplace(pair2, SdaOfferBalance{ Amount(100), Amount(10), Amount(100), Amount(10), Height(2) });
        entry.expiredSdaOfferBalances()[Height(2)].emplace(pair3, SdaOfferBalance{ Amount(100), Amount(10), Amount(100), Amount(10), Height(3) });

		auto expectedEntry = SdaExchangeEntry(Key());
        expectedEntry.sdaOfferBalances().emplace(pair1, SdaOfferBalance{ Amount(100), Amount(10), Amount(100), Amount(10), Height(2) });
		expectedEntry.sdaOfferBalances().emplace(pair4, SdaOfferBalance{ Amount(300), Amount(30), Amount(300), Amount(30), Height(3) });
        expectedEntry.sdaOfferBalances().emplace(pair2, SdaOfferBalance{ Amount(100), Amount(10), Amount(100), Amount(10), Height(2) });
        expectedEntry.expiredSdaOfferBalances()[Height(2)].emplace(pair3, SdaOfferBalance{ Amount(100), Amount(10), Amount(100), Amount(10), Height(3) });

		// Act:
		entry.unexpireOffers(
			Height(2),
			[&unexpireSdaOfferBalanceCalled](const auto& ) {
				unexpireSdaOfferBalanceCalled = true;
			}
		);

		// Assert:
		test::AssertEqualSdaExchangeData(expectedEntry, entry);
		EXPECT_TRUE(unexpireSdaOfferBalanceCalled);
	}

	TEST(TEST_CLASS, SdaOfferExists) {
		// Arrange:
		auto entry = SdaExchangeEntry(Key());
        MosaicsPair pair1{MosaicId(1), MosaicId(1)};
        MosaicsPair pair2{MosaicId(2), MosaicId(2)};
        MosaicsPair pair3{MosaicId(3), MosaicId(3)};
		entry.sdaOfferBalances().emplace(pair1, SdaOfferBalance{ Amount(100), Amount(10), Amount(100), Amount(10), Height(2) });
        entry.sdaOfferBalances().emplace(pair2, SdaOfferBalance{ Amount(200), Amount(20), Amount(200), Amount(20), Height(3) });

		// Assert:
		EXPECT_TRUE(entry.offerExists(pair1));
		EXPECT_TRUE(entry.offerExists(pair2));
		EXPECT_FALSE(entry.offerExists(pair3));
	}

	TEST(TEST_CLASS, PlaceSdaOffer) {
		// Arrange:
		auto entry = SdaExchangeEntry(Key());
		model::SdaOfferWithDuration sdaOffer1{ {{ UnresolvedMosaicId(1), Amount(10) }, { UnresolvedMosaicId(2), Amount(100) }}, BlockDuration(100)};
	    model::SdaOfferWithDuration sdaOffer2{ {{ UnresolvedMosaicId(2), Amount(100) }, { UnresolvedMosaicId(1), Amount(10) }}, BlockDuration(100)};

		auto expectedEntry = SdaExchangeEntry(Key());
        MosaicsPair pair1{MosaicId(1), MosaicId(1)};
        MosaicsPair pair2{MosaicId(2), MosaicId(2)};
		expectedEntry.sdaOfferBalances().emplace(pair1, SdaOfferBalance{ Amount(10), Amount(100), Amount(10), Amount(100), Height(1) });
        expectedEntry.sdaOfferBalances().emplace(pair2, SdaOfferBalance{ Amount(100), Amount(10), Amount(100), Amount(10), Height(2) });

		// Act:
		entry.addOffer(MosaicId(1), MosaicId(1), &sdaOffer1, Height(1));
		entry.addOffer(MosaicId(2), MosaicId(2), &sdaOffer2, Height(2));

		// Assert:
		test::AssertEqualSdaExchangeData(expectedEntry, entry);
	}

	TEST(TEST_CLASS, RemoveSdaOffer) {
		// Arrange:
		auto entry = SdaExchangeEntry(Key());
        MosaicsPair pair1{MosaicId(1), MosaicId(1)};
        MosaicsPair pair2{MosaicId(2), MosaicId(2)};
        MosaicsPair pair3{MosaicId(3), MosaicId(3)};
        MosaicsPair pair4{MosaicId(4), MosaicId(4)};
        entry.sdaOfferBalances().emplace(pair1, SdaOfferBalance{ Amount(100), Amount(10), Amount(100), Amount(10), Height(1) });
        entry.sdaOfferBalances().emplace(pair2, SdaOfferBalance{ Amount(200), Amount(20), Amount(200), Amount(20), Height(2) });
        entry.sdaOfferBalances().emplace(pair3, SdaOfferBalance{ Amount(300), Amount(30), Amount(300), Amount(30), Height(2) });

		auto expectedEntry = SdaExchangeEntry(Key());
		expectedEntry.sdaOfferBalances().emplace(pair1, SdaOfferBalance{ Amount(100), Amount(10), Amount(100), Amount(10), Height(1) });
        expectedEntry.sdaOfferBalances().emplace(pair2, SdaOfferBalance{ Amount(200), Amount(20), Amount(200), Amount(20), Height(2) });

		// Act:
		entry.removeOffer(pair3);
		entry.removeOffer(pair4);

		// Assert:
		test::AssertEqualSdaExchangeData(expectedEntry, entry);
	}

	TEST(TEST_CLASS, CannotGetSdaOfferWhenSdaOfferDoesntExist) {
		// Arrange:
		auto entry = SdaExchangeEntry(Key());
        MosaicsPair pair1{MosaicId(1), MosaicId(1)};
        MosaicsPair pair2{MosaicId(2), MosaicId(2)};
		entry.sdaOfferBalances().emplace(pair2, SdaOfferBalance{ test::GenerateSdaOfferBalance()});

		// Assert:
		EXPECT_THROW(entry.getSdaOfferBalance(pair1), catapult_invalid_argument);
	}

	TEST(TEST_CLASS, CanGetSdaOffer) {
		// Arrange:
		auto entry = SdaExchangeEntry(Key());
		auto offer = test::GenerateSdaOfferBalance();
		MosaicsPair pair{MosaicId(1), MosaicId(1)};
        entry.sdaOfferBalances().emplace(pair, offer);

		// Assert:
		test::AssertSdaOfferBalance(offer, entry.getSdaOfferBalance(pair));
	}

	TEST(TEST_CLASS, Empty) {
		// Arrange:
		auto entry = SdaExchangeEntry(Key());
        MosaicsPair pair1{MosaicId(1), MosaicId(1)};
        MosaicsPair pair2{MosaicId(2), MosaicId(2)};

		// Assert:
		EXPECT_TRUE(entry.empty());        
		entry.sdaOfferBalances().emplace(pair1, SdaOfferBalance{ test::GenerateSdaOfferBalance() });
		EXPECT_FALSE(entry.empty());
		entry.sdaOfferBalances().erase(pair1);
		entry.sdaOfferBalances().emplace(pair2, SdaOfferBalance{ test::GenerateSdaOfferBalance() });
		EXPECT_FALSE(entry.empty());
		entry.sdaOfferBalances().erase(pair2);
		entry.expiredSdaOfferBalances()[Height(2)].emplace(pair1, SdaOfferBalance{ test::GenerateSdaOfferBalance() });
		EXPECT_FALSE(entry.empty());
		entry.expiredSdaOfferBalances().erase(Height(2));
		entry.expiredSdaOfferBalances()[Height(2)].emplace(pair2, SdaOfferBalance{ test::GenerateSdaOfferBalance() });
		EXPECT_FALSE(entry.empty());
	}
}}
