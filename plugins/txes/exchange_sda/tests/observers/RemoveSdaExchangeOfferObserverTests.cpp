/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/cache_core/AccountStateCache.h"
#include "src/observers/Observers.h"
#include "tests/test/SdaExchangeTestUtils.h"
#include "tests/test/plugins/ObserverTestUtils.h"
#include "tests/TestHarness.h"
#include <boost/lexical_cast.hpp>

namespace catapult { namespace observers {

#define TEST_CLASS RemoveSdaExchangeOfferObserverTests

    DEFINE_COMMON_OBSERVER_TESTS(RemoveSdaExchangeOfferV1,)

    namespace {
        using ObserverTestContext = test::ObserverTestContextT<test::SdaExchangeCacheFactory>;

        struct RemoveSdaExchangeOfferValues {
            public:
            explicit RemoveSdaExchangeOfferValues(
                    Height&& initialExpiryHeight,
                    Height&& expectedExpiryHeight,
                    std::vector<model::SdaOfferMosaic>&& offersToRemove,
                    state::SdaExchangeEntry&& initialSdaExchangeEntry,
                    state::SdaExchangeEntry&& expectedSdaExchangeEntry,
                    std::vector<state::SdaOfferGroupEntry>&& initialSdaOfferGroupEntries,
                    std::vector<state::SdaOfferGroupEntry>&& expectedSdaOfferGroupEntries)
                : InitialExpiryHeight(std::move(initialExpiryHeight))
                , ExpectedExpiryHeight(std::move(expectedExpiryHeight))
                , OffersToRemove(std::move(offersToRemove))
                , InitialSdaExchangeEntry(std::move(initialSdaExchangeEntry))
                , ExpectedSdaExchangeEntry(std::move(expectedSdaExchangeEntry))
                , InitialSdaOfferGroupEntries(std::move(initialSdaOfferGroupEntries))
                , ExpectedSdaOfferGroupEntries(std::move(expectedSdaOfferGroupEntries))
            {}

        public:
            Height InitialExpiryHeight;
            Height ExpectedExpiryHeight;
            std::vector<model::SdaOfferMosaic> OffersToRemove;
            state::SdaExchangeEntry InitialSdaExchangeEntry;
            state::SdaExchangeEntry ExpectedSdaExchangeEntry;
            std::vector<state::SdaOfferGroupEntry> InitialSdaOfferGroupEntries;
            std::vector<state::SdaOfferGroupEntry> ExpectedSdaOfferGroupEntries;
        };

        template<typename Notification>
        std::unique_ptr<Notification> CreateNotification(const RemoveSdaExchangeOfferValues& values) {
            return std::make_unique<Notification>(
                values.InitialSdaExchangeEntry.owner(),
                values.OffersToRemove.size(),
                values.OffersToRemove.data());
        }

        void PrepareEntries(state::SdaExchangeEntry& sdaExchangeEntry1, state::SdaExchangeEntry& sdaExchangeEntry2, std::vector<state::SdaOfferGroupEntry>& sdaOfferGroupEntries1, std::vector<state::SdaOfferGroupEntry>& sdaOfferGroupEntries2, const Key& owner) {
            state::MosaicsPair pair1 {MosaicId(1), MosaicId(2)};
            state::MosaicsPair pair2 {MosaicId(2), MosaicId(1)};

            sdaExchangeEntry1.sdaOfferBalances().emplace(pair1,
                state::SdaOfferBalance {Amount(10), Amount(100), Amount(10), Amount(100), Height(15)});
            sdaExchangeEntry1.sdaOfferBalances().emplace(pair2,
                state::SdaOfferBalance {Amount(20), Amount(200), Amount(20), Amount(200), Height(5)});         
            for (auto offer : sdaExchangeEntry1.sdaOfferBalances()) {
                std::string reduced = reducedFraction(offer.second.InitialMosaicGive, offer.second.InitialMosaicGet);
                auto groupHash = calculateGroupHash(offer.first.first, offer.first.second, reduced);
                state::SdaOfferBasicInfo info{ owner, offer.second.CurrentMosaicGive, offer.second.Deadline };
                state::SdaOfferGroupEntry sdaOfferGroupEntry(groupHash);          
                sdaOfferGroupEntry.sdaOfferGroup().emplace_back(info);
                sdaOfferGroupEntries1.emplace_back(sdaOfferGroupEntry);
            }

            sdaExchangeEntry2.expiredSdaOfferBalances().emplace(Height(1), sdaExchangeEntry1.sdaOfferBalances());
            for (auto offer : sdaExchangeEntry2.sdaOfferBalances()) {
                std::string reduced = reducedFraction(offer.second.InitialMosaicGive, offer.second.InitialMosaicGet);
                auto groupHash = calculateGroupHash(offer.first.first, offer.first.second, reduced);
                state::SdaOfferBasicInfo info{ owner, offer.second.CurrentMosaicGive, offer.second.Deadline };
                state::SdaOfferGroupEntry sdaOfferGroupEntry(groupHash);          
                sdaOfferGroupEntry.sdaOfferGroup().emplace_back(info);
                sdaOfferGroupEntries2.emplace_back(sdaOfferGroupEntry);
            }          
        }

        template<typename TTraits>
        void RunTest(const RemoveSdaExchangeOfferValues& values) {
            // Arrange:
            Height currentHeight(1);
            ObserverTestContext context(NotifyMode::Commit, currentHeight);
            auto pNotification = CreateNotification<model::RemoveSdaOfferNotification<TTraits::Version>>(values);
            auto pObserver = TTraits::CreateObserver();
            auto& accountStateCache = context.cache().sub<cache::AccountStateCache>();
            auto& sdaExchangeCache = context.cache().sub<cache::SdaExchangeCache>();
            auto& sdaOfferGroupCache = context.cache().sub<cache::SdaOfferGroupCache>();
            const auto& owner = values.InitialSdaExchangeEntry.owner();

            // Populate cache.
            sdaExchangeCache.insert(values.InitialSdaExchangeEntry);
            for (auto& val : values.InitialSdaOfferGroupEntries)
                sdaOfferGroupCache.insert(val);
            test::AddAccountState(accountStateCache, sdaExchangeCache, owner, currentHeight, values.OffersToRemove);

            // Act:
            test::ObserveNotification(*pObserver, *pNotification, context);

            // Assert: check the cache
            auto exchangeIter = sdaExchangeCache.find(owner);
            const auto& exchangeEntry = exchangeIter.get();
            test::AssertEqualSdaExchangeData(exchangeEntry, values.ExpectedSdaExchangeEntry);
            EXPECT_EQ(values.InitialSdaExchangeEntry.minExpiryHeight(), values.InitialExpiryHeight);
            EXPECT_EQ(exchangeEntry.minExpiryHeight(), values.ExpectedExpiryHeight);

            auto accountIter = accountStateCache.find(owner);
            const auto& accountEntry = accountIter.get();
            EXPECT_EQ(Amount(20), accountEntry.Balances.get(MosaicId(1)));
            EXPECT_EQ(Amount(40), accountEntry.Balances.get(MosaicId(2)));
        }


        auto CreateRemoveSdaExchangeOfferValues(state::SdaExchangeEntry&& initialSdaExchangeEntry, state::SdaExchangeEntry&& expectedSdaExchangeEntry, std::vector<state::SdaOfferGroupEntry>&& initialSdaOfferGroupEntries, std::vector<state::SdaOfferGroupEntry>&& expectedSdaOfferGroupEntries) {
            return RemoveSdaExchangeOfferValues(
                Height(5),
                Height(0),
                {
                    model::SdaOfferMosaic{test::UnresolveXor(MosaicId(1)), test::UnresolveXor(MosaicId(2))},
                    model::SdaOfferMosaic{test::UnresolveXor(MosaicId(2)), test::UnresolveXor(MosaicId(1))},
                },
                std::move(initialSdaExchangeEntry),
                std::move(expectedSdaExchangeEntry),
                std::move(initialSdaOfferGroupEntries),
                std::move(expectedSdaOfferGroupEntries));
        }

        struct RemoveSdaExchangeOfferObserverV1Traits {
            static constexpr VersionType Version = 1;
            static auto CreateObserver() {
                return CreateRemoveSdaExchangeOfferV1Observer();
            }
        };
    }

#define TRAITS_BASED_TEST(TEST_NAME) \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(); \
	TEST(TEST_CLASS, TEST_NAME##_v1) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<RemoveSdaExchangeOfferObserverV1Traits>(); } \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)()

    TRAITS_BASED_TEST(RemoveOffer_Commit) {
		// Arrange:
		auto offerOwner = test::GenerateRandomByteArray<Key>();
        state::SdaExchangeEntry initialSdaExchangeEntry(offerOwner);
        state::SdaExchangeEntry expectedSdaExchangeEntry(offerOwner);
        std::vector<state::SdaOfferGroupEntry> initialSdaOfferGroupEntries;
        std::vector<state::SdaOfferGroupEntry> expectedSdaOfferGroupEntries;
        PrepareEntries(initialSdaExchangeEntry, expectedSdaExchangeEntry, initialSdaOfferGroupEntries, expectedSdaOfferGroupEntries, offerOwner);
        auto values = CreateRemoveSdaExchangeOfferValues(
            std::move(initialSdaExchangeEntry), std::move(expectedSdaExchangeEntry),
            std::move(initialSdaOfferGroupEntries), std::move(expectedSdaOfferGroupEntries));

		// Assert:
		RunTest<TTraits>(values);
	}
}}
