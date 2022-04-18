/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/observers/Observers.h"
#include "tests/test/SdaExchangeTestUtils.h"
#include "tests/test/plugins/ObserverTestUtils.h"
#include "tests/TestHarness.h"
#include <boost/lexical_cast.hpp>

namespace catapult { namespace observers {

#define TEST_CLASS PlaceSdaExchangeOfferObserverTests

    DEFINE_COMMON_OBSERVER_TESTS(PlaceSdaExchangeOfferV1, )

    namespace {
        using ObserverTestContext = test::ObserverTestContextT<test::SdaExchangeCacheFactory>;

        auto CreateConfig() {
			test::MutableBlockchainConfiguration config;
			auto sdaExchangeConfig = config::SdaExchangeConfiguration::Uninitialized();
            sdaExchangeConfig.OfferSortPolicy = SortPolicy::Default;
			config.Network.SetPluginConfiguration(sdaExchangeConfig);

			return config.ToConst();
		}

        struct PlaceSdaExchangeOfferValues {
        public:
            explicit PlaceSdaExchangeOfferValues(
                    Key&& owner,
                    std::vector<model::SdaOfferWithOwnerAndDuration>&& offers)
                : Owner(std::move(owner))
                , SdaOffers(std::move(offers))
            {}

        public:
            Key Owner;
            std::vector<model::SdaOfferWithOwnerAndDuration> SdaOffers;
        };

        template<typename Notification>
        std::unique_ptr<Notification> CreateNotification(const PlaceSdaExchangeOfferValues& values) {
            return std::make_unique<Notification>(
                values.Owner,
                values.SdaOffers.size(),
                values.SdaOffers.data());
        }

        Height GetOfferDeadline(const BlockDuration& duration, const Height& currentHeight) {
            auto maxDeadline = std::numeric_limits<Height::ValueType>::max();
            return Height((duration.unwrap() < maxDeadline - currentHeight.unwrap()) ? currentHeight.unwrap() + duration.unwrap() : maxDeadline);
        }

        template<typename TTraits>
        void RunTest(const PlaceSdaExchangeOfferValues& expectedValues) {
            // Arrange:
            Height currentHeight(1);
            ObserverTestContext context(NotifyMode::Commit, currentHeight, CreateConfig());
            auto pNotification = CreateNotification<model::PlaceSdaOfferNotification<TTraits::Version>>(expectedValues);
            auto pObserver = TTraits::CreateObserver();
            auto& sdaExchangeCache = context.cache().sub<cache::SdaExchangeCache>();
            auto& sdaOfferGroupCache = context.cache().sub<cache::SdaOfferGroupCache>();

            // Populate cache.
            state::SdaExchangeEntry sdaExchangeEntry(expectedValues.Owner, TTraits::Version);
            for (const auto& offer : expectedValues.SdaOffers) {
                auto mosaicIdGive = context.observerContext().Resolvers.resolve(offer.MosaicGive.MosaicId);
                auto mosaicIdGet = context.observerContext().Resolvers.resolve(offer.MosaicGet.MosaicId);
                auto deadline = GetOfferDeadline(offer.Duration, context.observerContext().Height);
                sdaExchangeEntry.sdaOfferBalances().emplace(state::MosaicsPair{mosaicIdGive, mosaicIdGet}, state::SdaOfferBalance{offer.MosaicGive.Amount, offer.MosaicGet.Amount, offer.MosaicGive.Amount, offer.MosaicGet.Amount, deadline});

                std::string reduced = reducedFraction(offer.MosaicGive.Amount, offer.MosaicGet.Amount);
                auto groupHash = calculateGroupHash(mosaicIdGive, mosaicIdGet, reduced);
                state::SdaOfferGroupEntry sdaOfferGroupEntry(groupHash);
                if (!sdaOfferGroupCache.contains(groupHash))
                    sdaOfferGroupCache.insert(sdaOfferGroupEntry);
                state::SdaOfferBasicInfo info{ expectedValues.Owner, offer.MosaicGive.Amount, deadline };
                sdaOfferGroupEntry.sdaOfferGroup().emplace_back(info);
            }
            sdaExchangeCache.insert(sdaExchangeEntry);

            // Act:
            test::ObserveNotification(*pObserver, *pNotification, context);

            // Assert: check the cache
            EXPECT_TRUE(sdaExchangeCache.contains(expectedValues.Owner));
            auto iter = sdaExchangeCache.find(expectedValues.Owner);
            const auto& entry = iter.get();
            EXPECT_EQ(entry.sdaOfferBalances().size(), expectedValues.SdaOffers.size());
            for (const auto& offer : expectedValues.SdaOffers) {
                auto mosaicIdGive = context.observerContext().Resolvers.resolve(offer.MosaicGive.MosaicId);
                auto mosaicIdGet = context.observerContext().Resolvers.resolve(offer.MosaicGet.MosaicId);
                auto deadline = GetOfferDeadline(offer.Duration, context.observerContext().Height);
                state::MosaicsPair pair {mosaicIdGive, mosaicIdGet};
                state::SdaOfferBalance sdaOffer{offer.MosaicGive.Amount, offer.MosaicGet.Amount, offer.MosaicGive.Amount, offer.MosaicGet.Amount, deadline};
                test::AssertSdaOfferBalance(entry.sdaOfferBalances().at(pair), sdaOffer);
            }
        }

        PlaceSdaExchangeOfferValues CreatePlaceSdaExchangeOfferValues(Key&& owner) {
            return  PlaceSdaExchangeOfferValues(std::move(owner), {
                    model::SdaOfferWithOwnerAndDuration{model::SdaOffer{{UnresolvedMosaicId(1), Amount(10)}, {UnresolvedMosaicId(2), Amount(100)}}, owner, BlockDuration(1000)},
                    model::SdaOfferWithOwnerAndDuration{model::SdaOffer{{UnresolvedMosaicId(2), Amount(200)}, {UnresolvedMosaicId(1), Amount(20)}}, owner, BlockDuration(2000)},
                    model::SdaOfferWithOwnerAndDuration{model::SdaOffer{{UnresolvedMosaicId(3), Amount(30)}, {UnresolvedMosaicId(4), Amount(300)}}, owner, BlockDuration(3000)},
                    model::SdaOfferWithOwnerAndDuration{model::SdaOffer{{UnresolvedMosaicId(4), Amount(150)}, {UnresolvedMosaicId(3), Amount(15)}}, owner, BlockDuration(std::numeric_limits<Height::ValueType>::max())},
                }
            );
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
        auto values = CreatePlaceSdaExchangeOfferValues(std::move(test::GenerateRandomByteArray<Key>()));

        // Assert:
        RunTest<TTraits>(values);
    }
}}