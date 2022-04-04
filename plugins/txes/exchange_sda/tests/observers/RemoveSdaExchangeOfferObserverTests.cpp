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

namespace catapult { namespace observers {

#define TEST_CLASS RemoveSdaExchangeOfferObserverTests

    DEFINE_COMMON_OBSERVER_TESTS(RemoveSdaExchangeOfferV1)

    namespace {
        using ObserverTestContext = test::ObserverTestContextT<test::SdaExchangeCacheFactory>;

        struct RemoveSdaExchangeOfferValues {
            public:
            explicit RemoveSdaExchangeOfferValues(
                    Height&& initialExpiryHeight,
                    Height&& expectedExpiryHeight,
                    std::vector<model::SdaOfferMosaic>&& offersToRemove,
                    state::SdaExchangeEntry&& initialEntry,
                    state::SdaExchangeEntry&& expectedEntry)
                : InitialExpiryHeight(std::move(initialExpiryHeight))
                , ExpectedExpiryHeight(std::move(expectedExpiryHeight))
                , OffersToRemove(std::move(offersToRemove))
                , InitialEntry(std::move(initialEntry))
                , ExpectedEntry(std::move(expectedEntry))
            {}

        public:
            Height InitialExpiryHeight;
            Height ExpectedExpiryHeight;
            std::vector<model::SdaOfferMosaic> OffersToRemove;
            state::SdaExchangeEntry InitialEntry;
            state::SdaExchangeEntry ExpectedEntry;
        };

        template<typename Notification>
        std::unique_ptr<Notification> CreateNotification(RemoveSdaExchangeOfferValues& values) {
            return std::make_unique<Notification>(
                values.InitialEntry.owner(),
                values.OffersToRemove.size(),
                values.OffersToRemove.data());
        }

        void PrepareEntries(state::SdaExchangeEntry& entry1, state::SdaExchangeEntry& entry2) {
            state::MosaicsPair pair1 {MosaicId(1), MosaicId(2)};
            state::MosaicsPair pair2 {MosaicId(2), MosaicId(1)};
            entry1.sdaOfferBalances().emplace(pair1,
                state::SdaOfferBalance {Amount(10), Amount(100), Amount(10), Amount(100), Height(15)});
            entry1.sdaOfferBalances().emplace(pair2, 
                state::SdaOfferBalance {Amount(20), Amount(200), Amount(20), Amount(200), Height(5)});

            entry2.expiredSdaOfferBalances().emplace(Height(1), entry1.sdaOfferBalances());
        }

        auto CreateRemoveSdaExchangeOfferValues(state::SdaExchangeEntry&& initialEntry, state::SdaExchangeEntry&& expectedEntry) {
            return RemoveSdaExchangeOfferValues(
                Height(5),
                Height(0),
                {
                    model::SdaOfferMosaic{UnresolvedMosaicId(1), UnresolvedMosaicId(2)},
                    model::SdaOfferMosaic{UnresolvedMosaicId(2), UnresolvedMosaicId(1)},
                },
                std::move(initialEntry),
                std::move(expectedEntry));
        }

        template<typename TTraits>
        void RunTest(const RemoveSdaExchangeOfferValues& values) {
            // Arrange:
            Height currentHeight(1);
            ObserverTestContext context(NotifyMode::Commit, currentHeight);
            auto pNotification =  CreateNotification<model::RemoveSdaOfferNotification<TTraits::Version>>(values);
            auto pObserver = TTraits::CreateObserver();
            auto& accountCache = context.cache().sub<cache::AccountStateCache>();
            auto& exchangeCache = context.cache().sub<cache::SdaExchangeCache>();
            auto owner = values.InitialEntry.owner();

            // Populate cache.
            accountCache.addAccount(owner, currentHeight);
            exchangeCache.insert(values.InitialEntry);

            // Act:
            test::ObserveNotification(*pObserver, *pNotification, context);

            // Assert: check the cache
            auto exchangeIter = exchangeCache.find(owner);
            const auto& exchangeEntry = exchangeIter.get();
            test::AssertEqualExchangeData(exchangeEntry, values.ExpectedEntry);
            EXPECT_EQ(values.InitialEntry.minExpiryHeight(), values.InitialExpiryHeight);
            EXPECT_EQ(exchangeEntry.minExpiryHeight(), values.ExpectedExpiryHeight);

            auto accountIter = accountCache.find(owner);
            const auto& accountEntry = accountIter.get();
            EXPECT_EQ(Amount(0), accountEntry.Balances.get(MosaicId(1)));
            EXPECT_EQ(Amount(20), accountEntry.Balances.get(MosaicId(2)));
        }
    }

    struct RemoveSdaExchangeOfferObserverV1Traits {
		static constexpr VersionType Version = 1;
		static auto CreateObserver() {
			return CreateRemoveSdaExchangeOfferV1Observer();
		}
	};

#define TRAITS_BASED_TEST(TEST_NAME) \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(); \
	TEST(TEST_CLASS, TEST_NAME##_v1) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<RemoveSdaExchangeOfferObserverV1Traits>(); } \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)()

    TRAITS_BASED_TEST(RemoveOffer_Commit) {
		// Arrange:
		auto offerOwner = test::GenerateRandomByteArray<Key>();
		state::ExchangeEntry initialEntry(offerOwner);
		state::ExchangeEntry expectedEntry(offerOwner);
		PrepareEntries(initialEntry, expectedEntry);
		auto values = CreateRemoveSdaExchangeOfferValues(std::move(initialEntry), std::move(expectedEntry));

		// Assert:
		RunTest<TTraits>(values);
	}
}}
