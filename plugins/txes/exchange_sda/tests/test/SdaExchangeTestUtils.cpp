/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "SdaExchangeTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace test {

	state::SdaOfferBalance GenerateSdaOfferBalance() {
		return state::SdaOfferBalance{
			test::GenerateRandomValue<Amount>(),
			test::GenerateRandomValue<Amount>(),
			test::GenerateRandomValue<Amount>(),
            test::GenerateRandomValue<Amount>(),
			test::GenerateRandomValue<Height>(),
		};
	}

	state::SdaExchangeEntry CreateSdaExchangeEntry(uint8_t sdaOfferCount, uint8_t expiredSdaOfferCount, Key key, VersionType version) {
		state::SdaExchangeEntry entry(key, version);
		for (uint8_t i = 1; i <= sdaOfferCount; ++i) {
            entry.sdaOfferBalances().emplace(MosaicId(i), state::SdaOfferBalance{test::GenerateSdaOfferBalance()});
		}
		for (uint8_t i = 1; i <= expiredSdaOfferCount; ++i) {
			state::SdaOfferBalanceMap sdaOfferBalances;
			sdaOfferBalances.emplace(MosaicId(i), state::SdaOfferBalance{test::GenerateSdaOfferBalance()});
			entry.expiredSdaOfferBalances().emplace(Height(i), sdaOfferBalances);
		}
		return entry;
	}

	void AssertSdaOffer(const model::SdaOffer& offer1, const model::SdaOffer& offer2) {
		EXPECT_EQ(offer1.MosaicGive.MosaicId, offer2.MosaicGive.MosaicId);
		EXPECT_EQ(offer1.MosaicGive.Amount, offer2.MosaicGive.Amount);
		EXPECT_EQ(offer1.MosaicGet.MosaicId, offer2.MosaicGet.MosaicId);
		EXPECT_EQ(offer1.MosaicGet.Amount, offer2.MosaicGet.Amount);
	}

	void AssertSdaOfferBalance(const state::SdaOfferBalance& offer1, const state::SdaOfferBalance& offer2) {
		EXPECT_EQ(offer1.CurrentMosaicGive, offer2.CurrentMosaicGive);
		EXPECT_EQ(offer1.CurrentMosaicGet, offer2.CurrentMosaicGet);
		EXPECT_EQ(offer1.InitialMosaicGive, offer2.InitialMosaicGive);
		EXPECT_EQ(offer1.InitialMosaicGet, offer2.InitialMosaicGet);
	}

	namespace {
		template<typename TOfferMap>
		void AssertOffers(const TOfferMap& offers1, const TOfferMap& offers2) {
			ASSERT_EQ(offers1.size(), offers2.size());
			for (const auto& pair : offers1) {
				AssertSdaOfferBalance(pair.second, offers2.find(pair.first)->second);
			}
		}

		template<typename TExpiredOfferMap>
		void AssertExpiredOffers(const TExpiredOfferMap& offers1, const TExpiredOfferMap& offers2) {
			ASSERT_EQ(offers1.size(), offers2.size());
			for (const auto& pair : offers1) {
				AssertSdaOfferBalance(pair.second, offers2.find(pair.first)->second);
			}
		}
	}

	void AssertEqualExchangeData(const state::SdaExchangeEntry& entry1, const state::SdaExchangeEntry& entry2) {
		EXPECT_EQ(entry1.version(), entry2.version());
		EXPECT_EQ(entry1.owner(), entry2.owner());
		AssertOffers(entry1.sdaOfferBalances(), entry2.sdaOfferBalances());
		AssertExpiredOffers(entry1.expiredSdaOfferBalances(), entry2.expiredSdaOfferBalances());
	}
}}


