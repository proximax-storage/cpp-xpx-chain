/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "SdaExchangeMapperTestUtils.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/tests/test/MapperTestUtils.h"
#include "tests/TestHarness.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace test {

    namespace {
        void AssertSdaOfferBalance(const state::SdaOfferBalance& offer, const bsoncxx::document::element& dbOffer) {
            EXPECT_EQ(offer.CurrentMosaicGive.unwrap(), GetUint64(dbOffer, "currentMosaicGiveAmount"));
            EXPECT_EQ(offer.CurrentMosaicGet.unwrap(), GetUint64(dbOffer, "currentMosaicGetAmount"));
            EXPECT_EQ(offer.InitialMosaicGive.unwrap(), GetUint64(dbOffer, "initialMosaicGiveAmount"));
            EXPECT_EQ(offer.InitialMosaicGet.unwrap(), GetUint64(dbOffer, "initialMosaicGetAmount"));
            EXPECT_EQ(offer.Deadline.unwrap(), GetUint64(dbOffer, "deadline"));
        }

        void AssertSdaOfferBalances(const state::SdaOfferBalanceMap& offers, const bsoncxx::document::view& dbOffers) {
            ASSERT_EQ(offers.size(), test::GetFieldCount(dbOffers));
            for (const auto& dbOffer : dbOffers) {
                auto mosaicIdGive = GetMosaicId(dbOffer, "mosaicIdGive");
                auto mosaicIdGet = GetMosaicId(dbOffer, "mosaicIdGet");
                state::MosaicsPair pair{mosaicIdGive, mosaicIdGet};
                auto& offer = offers.at(pair);
                AssertSdaOfferBalance(offer, dbOffer);
            }
        }

        void AssertExpiredSdaOfferBalances(const state::ExpiredSdaOfferBalanceMap& expiredOffers, const bsoncxx::document::view& dbOffers) {
            ASSERT_EQ(expiredOffers.size(), test::GetFieldCount(dbOffers));

            for (const auto& dbOffer : dbOffers) {
                auto height = Height{GetUint64(dbOffer, "height")};
                auto& offers = expiredOffers.at(height);
                AssertSdaOfferBalances(offers, dbOffer["sdaOfferBalances"].get_array().value);
            }
        }
    }

    void AssertEqualSdaExchangeData(const state::SdaExchangeEntry& entry, const Address& address, const bsoncxx::document::view& dbSdaExchangeEntry) {
        EXPECT_EQ(5u, test::GetFieldCount(dbSdaExchangeEntry));

        EXPECT_EQ(entry.owner(), GetKeyValue(dbSdaExchangeEntry, "owner"));
        EXPECT_EQ(address, test::GetAddressValue(dbSdaExchangeEntry, "ownerAddress"));
        EXPECT_EQ(entry.version(), GetUint32(dbSdaExchangeEntry, "version"));

        AssertSdaOfferBalances(entry.sdaOfferBalances(), dbSdaExchangeEntry["sdaOfferBalances"].get_array().value);
        AssertExpiredSdaOfferBalances(entry.expiredSdaOfferBalances(), dbSdaExchangeEntry["expiredSdaOfferBalances"].get_array().value);
    }

    namespace {
        void AssertSdaOfferGroup(const state::SdaOfferGroupVector& offers, const bsoncxx::array::view& dbOffers) {
            ASSERT_EQ(offers.size(), test::GetFieldCount(dbOffers));
            auto i = 0u;
            for (const auto& dbOffer : dbOffers) {
                const auto& offer = offers[i++];
                EXPECT_EQ(offer.Owner, GetKeyValue(dbOffer, "owner"));
                EXPECT_EQ(offer.MosaicGive.unwrap(), GetUint64(dbOffer, "mosaicGiveAmount"));
                EXPECT_EQ(offer.Deadline.unwrap(), GetUint64(dbOffer, "deadline"));
            }
        }
    }

    void AssertEqualSdaOfferGroupData(const state::SdaOfferGroupEntry& entry, const bsoncxx::document::view& dbSdaOfferGroupEntry) {
        EXPECT_EQ(2u, test::GetFieldCount(dbSdaOfferGroupEntry));

        EXPECT_EQ(entry.groupHash(), GetHashValue(dbSdaOfferGroupEntry, "groupHash"));

        AssertSdaOfferGroup(entry.sdaOfferGroup(), dbSdaOfferGroupEntry["sdaOfferGroup"].get_array().value);
    }
}}