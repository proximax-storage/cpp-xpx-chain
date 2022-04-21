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
            entry.sdaOfferBalances().emplace(state::MosaicsPair{MosaicId(i), MosaicId(i)}, state::SdaOfferBalance{test::GenerateSdaOfferBalance()});
        }
        for (uint8_t i = 1; i <= expiredSdaOfferCount; ++i) {
            state::SdaOfferBalanceMap sdaOfferBalances;
            sdaOfferBalances.emplace(state::MosaicsPair{MosaicId(i), MosaicId(i)}, state::SdaOfferBalance{test::GenerateSdaOfferBalance()});
            entry.expiredSdaOfferBalances().emplace(Height(i), sdaOfferBalances);
        }
        return entry;
    }

    void AssertSdaOffer(const model::SdaOffer& offer1, const model::SdaOffer& offer2) {
        EXPECT_EQ(offer1.MosaicGive.MosaicId, offer2.MosaicGive.MosaicId);
        EXPECT_EQ(offer1.MosaicGive.Amount, offer2.MosaicGive.Amount);
        EXPECT_EQ(offer1.MosaicGive.MosaicId, offer2.MosaicGive.MosaicId);
        EXPECT_EQ(offer1.MosaicGive.Amount, offer2.MosaicGive.Amount);
    }

    void AssertSdaOfferBalance(const state::SdaOfferBalance& offer1, const state::SdaOfferBalance& offer2) {
        EXPECT_EQ(offer1.CurrentMosaicGive, offer2.CurrentMosaicGive);
        EXPECT_EQ(offer1.CurrentMosaicGet, offer2.CurrentMosaicGet);
        EXPECT_EQ(offer1.InitialMosaicGive, offer2.InitialMosaicGive);
        EXPECT_EQ(offer1.InitialMosaicGet, offer2.InitialMosaicGet);
        EXPECT_EQ(offer1.Deadline, offer2.Deadline);
    }

    namespace {
        void AssertOffers(const state::SdaOfferBalanceMap& offers1, const state::SdaOfferBalanceMap& offers2) {
            ASSERT_EQ(offers1.size(), offers2.size());
            for (const auto& pair : offers1) {
                AssertSdaOfferBalance(pair.second, offers2.find(pair.first)->second);
            }
        }

        void AssertExpiredOffers(const state::ExpiredSdaOfferBalanceMap& offers1, const state::ExpiredSdaOfferBalanceMap& offers2) {
            ASSERT_EQ(offers1.size(), offers2.size());
            for (const auto& pair : offers1) {
                AssertOffers(pair.second, offers2.find(pair.first)->second);
            }
        }
    }

    void AssertEqualSdaExchangeData(const state::SdaExchangeEntry& entry1, const state::SdaExchangeEntry& entry2) {
        EXPECT_EQ(entry1.version(), entry2.version());
        EXPECT_EQ(entry1.owner(), entry2.owner());
        AssertOffers(entry1.sdaOfferBalances(), entry2.sdaOfferBalances());
        AssertExpiredOffers(entry1.expiredSdaOfferBalances(), entry2.expiredSdaOfferBalances());
    }

    state::SdaOfferBasicInfo GenerateSdaOfferBasicInfo() {
        return state::SdaOfferBasicInfo{
            test::GenerateRandomByteArray<Key>(),
            test::GenerateRandomValue<Amount>(),
            test::GenerateRandomValue<Height>(),
        };
    }

    state::SdaOfferGroupEntry CreateSdaOfferGroupEntry(uint16_t offerCount, Hash256 groupHash) {
        state::SdaOfferGroupEntry entry(groupHash);
        for (uint16_t i = 1; i <= offerCount; ++i)
            entry.sdaOfferGroup().emplace_back(test::GenerateSdaOfferBasicInfo());
        return entry;
    }

    void AssertSdaOfferGroupInfo(const state::SdaOfferGroupVector& offerGroup1, const state::SdaOfferGroupVector& offerGroup2) {
        ASSERT_EQ(offerGroup1.size(), offerGroup2.size());
        for (auto i = 0u; i < offerGroup1.size(); ++i) {
            const auto &group1 = offerGroup1[i];
            const auto &group2 = offerGroup2[i];
            EXPECT_EQ(group1.Owner, group2.Owner);
            EXPECT_EQ(group1.MosaicGive, group2.MosaicGive);
            EXPECT_EQ(group1.Deadline, group2.Deadline);
        }
    }

    void AssertEqualSdaOfferGroupData(const state::SdaOfferGroupEntry& entry1, const state::SdaOfferGroupEntry& entry2) {
        EXPECT_EQ(entry1.groupHash(), entry2.groupHash());
        AssertSdaOfferGroupInfo(entry1.sdaOfferGroup(), entry2.sdaOfferGroup());
    }

    void AddAccountState(
            cache::AccountStateCacheDelta& accountStateCache,
            cache::SdaExchangeCacheDelta& sdaExchangeCache, 
            const Key& owner,
            const Height& height,
            const std::vector<model::SdaOfferMosaic>& mosaics){
        accountStateCache.addAccount(owner, height);
        auto accountStateIter = accountStateCache.find(owner);
        auto& accountState = accountStateIter.get();

        auto sdaExchangeIter = sdaExchangeCache.find(owner);
        auto& sdaExchange = sdaExchangeIter.get();

        for (auto& mosaic : mosaics) {
            auto resolver = test::CreateResolverContextXor();
            MosaicId mosaicIdGive = resolver.resolve(mosaic.MosaicIdGive);
            MosaicId mosaicIdGet = resolver.resolve(mosaic.MosaicIdGet);
            Amount mosaicAmountGive = sdaExchange.getSdaOfferBalance(state::MosaicsPair{mosaicIdGive, mosaicIdGet}).CurrentMosaicGive;
            accountState.Balances.credit(mosaicIdGive, mosaicAmountGive, height);
        }
    }
}}


