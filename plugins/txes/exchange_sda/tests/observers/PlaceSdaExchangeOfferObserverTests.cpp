/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/observers/Observers.h"
#include "tests/test/SdaExchangeTestUtils.h"
#include "tests/test/plugins/ObserverTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace observers {

#define TEST_CLASS PlaceSdaExchangeOfferObserverTests

    DEFINE_COMMON_OBSERVER_TESTS(PlaceSdaExchangeOfferV1, )

    namespace {
        using ObserverTestContext = test::ObserverTestContextT<test::SdaExchangeCacheFactory>;

        struct PlaceSdaExchangeOfferValues {
        public:
            explicit PlaceSdaExchangeOfferValues(
                    Height&& initialExpiryHeight,
                    Height&& expectedExpiryHeight,
                    std::vector<model::SdaOfferWithOwnerAndDuration>&& offers,
                    state::SdaExchangeEntry&& initialEntry,
                    state::SdaExchangeEntry&& expectedEntry)
                : InitialExpiryHeight(std::move(initialExpiryHeight))
                , ExpectedExpiryHeight(std::move(expectedExpiryHeight))
                , SdaOffers(std::move(offers))
                , InitialEntry(std::move(initialEntry))
                , ExpectedEntry(std::move(expectedEntry))
            {}

        public:
            Height InitialExpiryHeight;
            Height ExpectedExpiryHeight;
            std::vector<model::SdaOfferWithOwnerAndDuration> SdaOffers;
            state::SdaExchangeEntry InitialEntry;
            state::SdaExchangeEntry ExpectedEntry;
        };

        template<typename Notification>
        std::unique_ptr<Notification> CreateNotification(const PlaceSdaExchangeOfferValues& values) {
            return std::make_unique<Notification>(
                values.InitialEntry.owner(),
                values.SdaOffers.size(),
                values.SdaOffers.data());
        }

        Height GetOfferDeadline(const BlockDuration& duration, const Height& currentHeight) {
            auto maxDeadline = std::numeric_limits<Height::ValueType>::max();
            return Height((duration.unwrap() < maxDeadline - currentHeight.unwrap()) ? currentHeight.unwrap() + duration.unwrap() : maxDeadline);
        }

        void PrepareEntries(state::SdaExchangeEntry& entry1, state::SdaExchangeEntry& entry2) {
            state::MosaicsPair pair1 {MosaicId(1), MosaicId(2)};
            state::MosaicsPair pair2 {MosaicId(2), MosaicId(1)};
            entry1.sdaOfferBalances().emplace(pair1,
                state::SdaOfferBalance {Amount(10), Amount(100), Amount(10), Amount(100), Height(15)});
            entry1.sdaOfferBalances().emplace(pair2, 
                state::SdaOfferBalance {Amount(20), Amount(200), Amount(20), Amount(200), Height(5)});

            entry2.sdaOfferBalances().emplace(pair1,
                state::SdaOfferBalance {Amount(10), Amount(100), Amount(10), Amount(100), Height(15)});
            auto pair = entry2.expiredSdaOfferBalances().emplace(Height(1), entry1.sdaOfferBalances());
            pair.first->second.at(pair2).CurrentMosaicGive = Amount(0);
        }

        auto CreatePlaceSdaExchangeOfferValues(state::SdaExchangeEntry&& initialEntry, state::SdaExchangeEntry&& expectedEntry, const Key& offerOwner) {
            return PlaceSdaExchangeOfferValues(
                Height(5),
                Height(15),
                {
                    model::SdaOfferWithOwnerAndDuration{model::SdaOffer{{UnresolvedMosaicId(1), Amount(10)}, {UnresolvedMosaicId(2), Amount(10)}}, offerOwner, BlockDuration(1000)},
                    model::SdaOfferWithOwnerAndDuration{model::SdaOffer{{UnresolvedMosaicId(2), Amount(200)}, {UnresolvedMosaicId(1), Amount(20)}}, offerOwner, BlockDuration(2000)},
                },
                std::move(initialEntry),
                std::move(expectedEntry));
        }

        template<typename TTraits>
        void RunTest(const PlaceSdaExchangeOfferValues& values, std::initializer_list<Height> expiryHeights) {
            // Arrange:
            Height currentHeight(1);
            ObserverTestContext context(NotifyMode::Commit, currentHeight);
            auto pNotification = CreateNotification<model::PlaceSdaOfferNotification<TTraits::Version>>(values);
            auto pObserver = TTraits::CreateObserver();
            auto& delta = context.cache().sub<cache::SdaExchangeCache>();
            const auto& owner = values.InitialEntry.owner();

            // Populate cache.
            delta.insert(values.InitialEntry);

            // Act:
            test::ObserveNotification(*pObserver, *pNotification, context);

            // Assert: check the cache
            auto iter = delta.find(owner);
            const auto& entry = iter.get();
            test::AssertEqualExchangeData(entry, values.ExpectedEntry);
            EXPECT_EQ(values.InitialEntry.minExpiryHeight(), values.InitialExpiryHeight);
            EXPECT_EQ(entry.minExpiryHeight(), values.ExpectedExpiryHeight);
            for (const auto& offer : values.SdaOffers) {
                auto mosaicIdGive = context.observerContext().Resolvers.resolve(offer.MosaicGive.MosaicId);
                auto mosaicIdGet = context.observerContext().Resolvers.resolve(offer.MosaicGet.MosaicId);
                auto deadline = GetOfferDeadline(offer.Duration, context.observerContext().Height);
                state::SdaOfferBalance sdaOffer{offer.MosaicGive.Amount, offer.MosaicGet.Amount, offer.MosaicGive.Amount, offer.MosaicGet.Amount, deadline};
                state::MosaicsPair pair {mosaicIdGive, mosaicIdGet};
                test::AssertSdaOfferBalance(entry.sdaOfferBalances().at(pair), sdaOffer);
            }

            for (const auto& expiryHeight : expiryHeights) {
                auto keys = delta.expiringOfferOwners(expiryHeight);
                EXPECT_EQ(1, keys.size());
                EXPECT_EQ(owner, *keys.begin());
            }
        }

        struct PlaceSdaExchangeOfferObserverV1Traits {
            static constexpr VersionType Version = 1;
            static auto CreateObserver() {
                return CreatePlaceSdaExchangeOfferV1Observer();
            }
        };
    }

#define TRAITS_BASED_TEST(TEST_NAME) \
    template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(); \
    TEST(TEST_CLASS, TEST_NAME##_v1) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<PlaceSdaExchangeOfferObserverV1Traits>(); } \
    template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)()

    TRAITS_BASED_TEST(PlaceSdaOffer_Commit) {
        // Arrange:
        auto offerOwner = test::GenerateRandomByteArray<Key>();
		state::SdaExchangeEntry initialEntry(offerOwner);
		state::SdaExchangeEntry expectedEntry(offerOwner);
		PrepareEntries(initialEntry, expectedEntry);
        auto values = CreatePlaceSdaExchangeOfferValues(std::move(initialEntry), std::move(expectedEntry), offerOwner);

        // Assert:
        RunTest<TTraits>(values, { Height(1), Height(15) });
    }
}}