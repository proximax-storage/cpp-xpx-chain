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
        constexpr uint32_t Max_Rollback_Blocks(50);
        constexpr auto Network_Identifier = model::NetworkIdentifier::Mijin_Test;

        auto CreateConfig() {
            test::MutableBlockchainConfiguration config;
            config.Network.MaxRollbackBlocks = Max_Rollback_Blocks;
            return config.ToConst();
        }

        struct CacheValues {
        public:
            explicit CacheValues(
                    std::vector<state::SdaExchangeEntry>&& initialEntries,
                    std::vector<state::SdaExchangeEntry>&& expectedEntries,
                    std::vector<state::AccountState>&& initialAccounts,
                    std::vector<state::AccountState>&& expectedAccounts)
                : InitialEntries(std::move(initialEntries))
                , ExpectedEntries(std::move(expectedEntries))
                , InitialAccounts(std::move(initialAccounts))
                , ExpectedAccounts(std::move(expectedAccounts))
            {}

        public:
            std::vector<state::SdaExchangeEntry> InitialEntries;
            std::vector<state::SdaExchangeEntry> ExpectedEntries;
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
                std::vector<state::AccountState>& initialAccounts,
                std::vector<state::AccountState>& expectedAccounts) {
            state::MosaicsPair pair1 {MosaicId(1), MosaicId(2)};
            state::MosaicsPair pair2 {MosaicId(2), MosaicId(1)};
            state::SdaOfferBalance offer1 {Amount(10), Amount(100), Amount(10), Amount(100), Current_Height + Height(5)};
            state::SdaOfferBalance offer2 {Amount(20), Amount(200), Amount(20), Amount(200), Current_Height};
            Height pruneHeight(Current_Height.unwrap() - Max_Rollback_Blocks);

            // SDA-SDA offers expired.
            state::SdaExchangeEntry initialEntry1(test::GenerateRandomByteArray<Key>());
            initialEntry1.sdaOfferBalances().emplace(pair1, offer1);
            offer1.Deadline = Current_Height;
            initialEntry1.sdaOfferBalances().emplace(pair2, offer2);
            initialEntries.push_back(initialEntry1);

            state::SdaExchangeEntry expectedEntry1(initialEntry1.owner());
            expectedEntry1.expiredSdaOfferBalances()[Current_Height].emplace(pair1, offer1);
            expectedEntry1.expiredSdaOfferBalances()[Current_Height].emplace(pair2, offer2);
            expectedEntries.push_back(expectedEntry1);

            initialAccounts.push_back(CreateAccount(initialEntry1.owner()));
            expectedAccounts.push_back(CreateAccount(initialEntry1.owner()));
            expectedAccounts.back().Balances.credit(pair1.first, Amount(100));
            expectedAccounts.back().Balances.credit(pair2.first, Amount(20));

            // SDA-SDA offers pruned along with the entry.
            state::SdaExchangeEntry initialEntry2(test::GenerateRandomByteArray<Key>());
            initialEntry2.expiredSdaOfferBalances()[pruneHeight].emplace(pair1, offer1);
            initialEntry2.expiredSdaOfferBalances()[pruneHeight].emplace(pair2, offer2);
            initialEntries.push_back(initialEntry2);
        }

        void RunTest(const CacheValues& values) {
            // Arrange:
            ObserverTestContext context(NotifyMode::Commit, Current_Height, CreateConfig());
            model::BlockNotification<1> notification = test::CreateBlockNotification();
            auto pObserver = CreateCleanupSdaOffersObserver();
            auto& exchangeCache = context.cache().sub<cache::SdaExchangeCache>();
            auto& accountCache = context.cache().sub<cache::AccountStateCache>();

            // Populate exchange cache.
            for (const auto& initialEntry : values.InitialEntries) {
                exchangeCache.insert(initialEntry);
                exchangeCache.addExpiryHeight(initialEntry.owner(), initialEntry.minExpiryHeight());
                exchangeCache.addExpiryHeight(initialEntry.owner(), initialEntry.minPruneHeight());
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
                const auto& entry = iter.get();
                test::AssertEqualSdaExchangeData(state::SdaExchangeEntry(entry.owner()), entry);
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
        std::vector<state::AccountState> initialAccounts;
        std::vector<state::AccountState> expectedAccounts;
        PrepareEntries(initialEntries, expectedEntries, initialAccounts, expectedAccounts);
        CacheValues values(std::move(initialEntries), std::move(expectedEntries), std::move(initialAccounts), std::move(expectedAccounts));

        // Assert:
		RunTest(values);
    }
}}