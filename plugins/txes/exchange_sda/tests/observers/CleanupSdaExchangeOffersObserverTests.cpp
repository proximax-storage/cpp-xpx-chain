/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/cache_core/AccountStateCache.h"
#include "catapult/model/Address.h"
#include "src/observers/Observers.h"
#include "tests/test/SdaExchangeTestUtils.h"
#include "tests/test/core/NotificationTestUtils.h"
#include "tests/test/other/MutableBlockchainConfiguration.h"
#include "tests/test/plugins/ObserverTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace observers {

#define TEST_CLASS CleanupSdaExchangeOffersObserverTests

    DEFINE_COMMON_OBSERVER_TESTS(CleanupSdaOffers,)

    namespace {
        using ObserverTestContext = test::ObserverTestContextT<test::SdaExchangeCacheFactory>;
        using Notification = model::BlockNotification<1>;

        constexpr Height Current_Height(55);
        constexpr auto Network_Identifier = model::NetworkIdentifier::Mijin_Test;

        struct CacheValues {
        public:
            explicit CacheValues(
                    std::vector<state::SdaExchangeEntry>&& initialEntries,
                    std::vector<state::SdaExchangeEntry>&& expectedEntries,
                    std::vector<state::SdaOfferGroupEntry>&& initialGroupEntries,
                    std::vector<state::SdaOfferGroupEntry>&& expectedGroupEntries,
                    std::vector<state::AccountState>&& initialAccounts,
                    std::vector<state::AccountState>&& expectedAccounts)
                : InitialEntries(std::move(initialEntries))
                , ExpectedEntries(std::move(expectedEntries))
                , InitialGroupEntries(std::move(initialGroupEntries))
                , ExpectedGroupEntries(std::move(expectedGroupEntries))
                , InitialAccounts(std::move(initialAccounts))
                , ExpectedAccounts(std::move(expectedAccounts))
            {}

        public:
            std::vector<state::SdaExchangeEntry> InitialEntries;
            std::vector<state::SdaExchangeEntry> ExpectedEntries;
            std::vector<state::SdaOfferGroupEntry> InitialGroupEntries;
            std::vector<state::SdaOfferGroupEntry> ExpectedGroupEntries;
            std::vector<state::AccountState> InitialAccounts;
            std::vector<state::AccountState> ExpectedAccounts;
        };

        state::AccountState CreateAccount(const Key& owner) {
            auto address = model::PublicKeyToAddress(owner, Network_Identifier);
            state::AccountState account(address, Height(1));
            account.PublicKey = owner;
            account.PublicKeyHeight = Height(1);
            return account;
        }

        void AssertAccount(const state::AccountState& expected, const state::AccountState& actual) {
            ASSERT_EQ(expected.Balances.size(), actual.Balances.size());
            for (auto iter = expected.Balances.begin(); iter != expected.Balances.end(); ++iter)
                EXPECT_EQ(iter->second, actual.Balances.get(iter->first));
        }

        void PrepareEntries(
                std::vector<state::SdaExchangeEntry>& initialEntries,
                std::vector<state::SdaExchangeEntry>& expectedEntries,
                std::vector<state::SdaOfferGroupEntry>& initialGroupEntries,
                std::vector<state::SdaOfferGroupEntry>& expectedGroupEntries,
                std::vector<state::AccountState>& initialAccounts,
                std::vector<state::AccountState>& expectedAccounts) {
            constexpr state::MosaicsPair pair1 {MosaicId(1), MosaicId(210)};
            constexpr state::MosaicsPair pair2 {MosaicId(210), MosaicId(1)};
            state::SdaOfferBalance offer1 {Amount(10), Amount(100), Amount(10), Amount(100), Current_Height + Height(5)};
            state::SdaOfferBalance offer2 {Amount(20), Amount(200), Amount(20), Amount(200), Current_Height};
            Height pruneHeight = Current_Height;

            // Some SDA-SDA offers expired.
            state::SdaExchangeEntry initialEntry1(test::GenerateRandomByteArray<Key>());
            initialEntry1.sdaOfferBalances().emplace(pair1, offer1);
            initialEntry1.sdaOfferBalances().emplace(pair2, offer2);
            initialEntries.push_back(initialEntry1);
            for (auto offer : initialEntry1.sdaOfferBalances()) {
                std::string reduced = reducedFraction(offer.second.InitialMosaicGive, offer.second.InitialMosaicGet);
                auto groupHash = calculateGroupHash(offer.first.first, offer.first.second, reduced);
                state::SdaOfferBasicInfo info{ initialEntry1.owner(), offer.second.CurrentMosaicGive, offer.second.Deadline };
                state::SdaOfferGroupEntry initialGroupEntry(groupHash);          
                initialGroupEntry.sdaOfferGroup().emplace_back(info);
                initialGroupEntries.push_back(initialGroupEntry);
            }

            state::SdaExchangeEntry expectedEntry1(initialEntry1.owner());
            expectedEntry1.sdaOfferBalances().emplace(pair1, offer1);
            expectedEntries.push_back(expectedEntry1);
            for (auto offer : expectedEntry1.sdaOfferBalances()) {
                std::string reduced = reducedFraction(offer.second.InitialMosaicGive, offer.second.InitialMosaicGet);
                auto groupHash = calculateGroupHash(offer.first.first, offer.first.second, reduced);
                state::SdaOfferBasicInfo info{ expectedEntry1.owner(), offer.second.CurrentMosaicGive, offer.second.Deadline };
                state::SdaOfferGroupEntry expectedGroupEntry(groupHash);          
                expectedGroupEntry.sdaOfferGroup().emplace_back(info);
                expectedGroupEntries.push_back(expectedGroupEntry);
            }

            initialAccounts.push_back(CreateAccount(initialEntry1.owner()));
            expectedAccounts.push_back(CreateAccount(initialEntry1.owner()));
            expectedAccounts.back().Balances.credit(pair2.first, Amount(20));

            // All SDA-SDA offers expired.
            state::SdaExchangeEntry initialEntry2(test::GenerateRandomByteArray<Key>());
            offer1.Deadline = Current_Height;
            initialEntry2.sdaOfferBalances().emplace(pair1, offer1);
            initialEntry2.sdaOfferBalances().emplace(pair2, offer2);
            initialEntries.push_back(initialEntry2);
            for (auto offer : initialEntry2.sdaOfferBalances()) {
                std::string reduced = reducedFraction(offer.second.InitialMosaicGive, offer.second.InitialMosaicGet);
                auto groupHash = calculateGroupHash(offer.first.first, offer.first.second, reduced);
                state::SdaOfferBasicInfo info{ initialEntry2.owner(), offer.second.CurrentMosaicGive, offer.second.Deadline };
                state::SdaOfferGroupEntry initialGroupEntry(groupHash);
                initialGroupEntry.sdaOfferGroup().emplace_back(info);
                initialGroupEntries.push_back(initialGroupEntry);
            }

            initialAccounts.push_back(CreateAccount(initialEntry2.owner()));
            expectedAccounts.push_back(CreateAccount(initialEntry2.owner()));
            expectedAccounts.back().Balances.credit(pair1.first, Amount(10));
            expectedAccounts.back().Balances.credit(pair2.first, Amount(20));

            // SDA-SDA offers pruned immediately along with the entry.
            state::SdaExchangeEntry initialEntry3(test::GenerateRandomByteArray<Key>());
            initialEntry3.expiredSdaOfferBalances()[pruneHeight].emplace(pair1, offer1);
            initialEntry3.expiredSdaOfferBalances()[pruneHeight].emplace(pair2, offer2);
            initialEntries.push_back(initialEntry3);
            for (auto expired : initialEntry3.expiredSdaOfferBalances()) {
                for (auto offer : expired.second){
                    std::string reduced = reducedFraction(offer.second.InitialMosaicGive, offer.second.InitialMosaicGet);
                    auto groupHash = calculateGroupHash(offer.first.first, offer.first.second, reduced);
                    state::SdaOfferBasicInfo info{ initialEntry3.owner(), offer.second.CurrentMosaicGive, offer.second.Deadline };
                    state::SdaOfferGroupEntry initialGroupEntry(groupHash);          
                    initialGroupEntry.sdaOfferGroup().emplace_back(info);
                    initialGroupEntries.push_back(initialGroupEntry);
                }
            }
        }

        void RunTest(const CacheValues& values) {
            // Arrange:
            ObserverTestContext context(NotifyMode::Commit, Current_Height);
            model::BlockNotification<1> notification = test::CreateBlockNotification();
            auto pObserver = CreateCleanupSdaOffersObserver();
            auto& exchangeCache = context.cache().sub<cache::SdaExchangeCache>();
            auto& groupCache = context.cache().sub<cache::SdaOfferGroupCache>();
            auto& accountCache = context.cache().sub<cache::AccountStateCache>();

            // Populate exchange cache.
            for (const auto& initialEntry : values.InitialEntries) {
                exchangeCache.insert(initialEntry);
                exchangeCache.addExpiryHeight(initialEntry.owner(), initialEntry.minExpiryHeight());
                exchangeCache.addExpiryHeight(initialEntry.owner(), initialEntry.minPruneHeight());
            }

            // Populate group cache.
            for (const auto& initialGroupEntry : values.InitialGroupEntries) {
                if (!groupCache.contains(initialGroupEntry.groupHash())){
                    groupCache.insert(initialGroupEntry);
                    continue;
                }
     
                auto groupIter = groupCache.find(initialGroupEntry.groupHash());
                auto& groupEntry = groupIter.get();
                for (const auto& group : initialGroupEntry.sdaOfferGroup()) {
                    groupEntry.sdaOfferGroup().emplace_back(group);
                }
            }

            // Populate account cache.
            for (const auto& initialAccount : values.InitialAccounts) {
                accountCache.addAccount(initialAccount);
            }

            // Act:
            test::ObserveNotification(*pObserver, notification, context);

            // Assert: check the cache
            for (const auto& expectedEntry : values.ExpectedEntries) {
                auto iter = exchangeCache.find(expectedEntry.owner());
                const auto& actualEntry = iter.get();
                test::AssertEqualSdaExchangeData(actualEntry, expectedEntry);
            }

            for (auto i = values.ExpectedEntries.size(); i < values.InitialEntries.size(); ++i) {
                auto iter = exchangeCache.find(values.InitialEntries[i].owner());
                ASSERT_EQ(iter.tryGet(), nullptr);
            }

            for (const auto& expectedGroupEntry : values.ExpectedGroupEntries) {
                auto iter = groupCache.find(expectedGroupEntry.groupHash());
                const auto& actualEntry = iter.get();
                test::AssertEqualSdaOfferGroupData(actualEntry, expectedGroupEntry);
            }

            for (auto i = values.ExpectedGroupEntries.size(); i < values.InitialGroupEntries.size(); ++i) {
                const auto initialEntry = values.InitialGroupEntries[i];
                auto iter = groupCache.find(initialEntry.groupHash());
                if (i%2==1){
                   ASSERT_EQ(iter.tryGet(), nullptr);
                   continue;
                }                
                const auto& groupEntry = iter.get();
                ASSERT_EQ(groupEntry.sdaOfferGroup().size(), 1);
            }

            for (const auto& expectedAccount : values.ExpectedAccounts) {
                auto iter = accountCache.find(expectedAccount.Address);
                const auto& actualAccount = iter.get();
                AssertAccount(expectedAccount, actualAccount);
            }
        }
    }

    TEST(TEST_CLASS, CleanupSdaExchangeOffers_Commit) {
        // Arrange:
        std::vector<state::SdaExchangeEntry> initialEntries;
        std::vector<state::SdaExchangeEntry> expectedEntries;
        std::vector<state::SdaOfferGroupEntry> initialGroupEntries;
        std::vector<state::SdaOfferGroupEntry> expectedGroupEntries;
        std::vector<state::AccountState> initialAccounts;
        std::vector<state::AccountState> expectedAccounts;
        PrepareEntries(initialEntries, expectedEntries, initialGroupEntries, expectedGroupEntries, initialAccounts, expectedAccounts);
        CacheValues values(std::move(initialEntries), std::move(expectedEntries), std::move(initialGroupEntries), std::move(expectedGroupEntries), std::move(initialAccounts), std::move(expectedAccounts));

        // Assert:
		RunTest(values);
    }
}}