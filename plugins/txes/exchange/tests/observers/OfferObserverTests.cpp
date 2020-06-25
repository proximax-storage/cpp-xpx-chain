/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/observers/Observers.h"
#include "tests/test/ExchangeTestUtils.h"
#include "tests/test/plugins/ObserverTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace observers {

#define TEST_CLASS OfferObserverTests

	DEFINE_COMMON_OBSERVER_TESTS(OfferV1, )
	DEFINE_COMMON_OBSERVER_TESTS(OfferV2, )
	DEFINE_COMMON_OBSERVER_TESTS(OfferV3, )

	namespace {
		using ObserverTestContext = test::ObserverTestContextT<test::ExchangeCacheFactory>;

		struct OfferValues {
		public:
			explicit OfferValues(Key&& owner, std::vector<model::OfferWithDuration>&& offers)
				: Owner(std::move(owner))
				, Offers(std::move(offers))
			{}

		public:
			Key Owner;
			std::vector<model::OfferWithDuration> Offers;
		};

		template<VersionType version>
		std::unique_ptr<model::OfferNotification<version>> CreateNotification(OfferValues& values) {
			return std::make_unique<model::OfferNotification<version>>(
				values.Owner,
				values.Offers.size(),
				values.Offers.data());
		}

		Height GetOfferDeadline(const BlockDuration& duration, const Height& currentHeight) {
			auto maxDeadline = std::numeric_limits<Height::ValueType>::max();
			return Height((duration.unwrap() < maxDeadline - currentHeight.unwrap()) ? currentHeight.unwrap() + duration.unwrap() : maxDeadline);
		}

		template<typename TTraits>
		void RunTest(
				NotifyMode mode,
				const model::OfferNotification<TTraits::Version>& notification,
				bool entryExists,
				const OfferValues& expectedValues) {
			// Arrange:
			Height currentHeight(1);
			ObserverTestContext context(mode, currentHeight);
			auto pObserver = TTraits::CreateObserver();
			auto& delta = context.cache().sub<cache::ExchangeCache>();

			if (NotifyMode::Rollback == mode) {
				// Populate cache.
				state::ExchangeEntry entry(notification.Owner, TTraits::Version);
				for (const auto& offer : expectedValues.Offers) {
					auto mosaicId = context.observerContext().Resolvers.resolve(offer.Mosaic.MosaicId);
					auto deadline = GetOfferDeadline(offer.Duration, context.observerContext().Height);
					state::OfferBase baseOffer{offer.Mosaic.Amount, offer.Mosaic.Amount, offer.Cost, deadline};
					if (model::OfferType::Buy == offer.Type) {
						entry.buyOffers().emplace(mosaicId, state::BuyOffer{baseOffer, offer.Cost});
					} else {
						entry.sellOffers().emplace(mosaicId, state::SellOffer{baseOffer});
					}
				}
				delta.insert(entry);
			}

			// Act:
			test::ObserveNotification(*pObserver, notification, context);

			// Assert: check the cache
			EXPECT_EQ(entryExists, delta.contains(expectedValues.Owner));
			if (entryExists) {
				auto iter = delta.find(expectedValues.Owner);
				const auto& entry = iter.get();
				EXPECT_EQ(entry.buyOffers().size() + entry.sellOffers().size(), expectedValues.Offers.size());
				for (const auto& offer : expectedValues.Offers) {
					auto mosaicId = context.observerContext().Resolvers.resolve(offer.Mosaic.MosaicId);
					auto deadline = GetOfferDeadline(offer.Duration, context.observerContext().Height);
					state::OfferBase baseOffer{offer.Mosaic.Amount, offer.Mosaic.Amount, offer.Cost, deadline};
					if (model::OfferType::Buy == offer.Type) {
						test::AssertOffer(entry.buyOffers().at(mosaicId), state::BuyOffer{baseOffer, offer.Cost});
					} else {
						test::AssertOffer(entry.sellOffers().at(mosaicId), state::SellOffer{baseOffer});
					}
				}
			}
		}

		OfferValues CreateOfferValues() {
			return  OfferValues(test::GenerateRandomByteArray<Key>(), {
					model::OfferWithDuration{model::Offer{{UnresolvedMosaicId(1), Amount(10)}, Amount(100), model::OfferType::Sell}, BlockDuration(1000)},
					model::OfferWithDuration{model::Offer{{UnresolvedMosaicId(2), Amount(20)}, Amount(200), model::OfferType::Buy}, BlockDuration(2000)},
					model::OfferWithDuration{model::Offer{{UnresolvedMosaicId(3), Amount(30)}, Amount(300), model::OfferType::Sell}, BlockDuration(3000)},
					model::OfferWithDuration{model::Offer{{UnresolvedMosaicId(4), Amount(40)}, Amount(400), model::OfferType::Buy}, BlockDuration(std::numeric_limits<Height::ValueType>::max())},
				}
			);
		}

		struct ObserverV1Traits {
			static constexpr VersionType Version = 1;
			static auto CreateObserver() {
				return CreateOfferV1Observer();
			}
		};

		struct ObserverV2Traits {
			static constexpr VersionType Version = 2;
			static auto CreateObserver() {
				return CreateOfferV2Observer();
			}
		};

		struct ObserverV3Traits {
			static constexpr VersionType Version = 3;
			static auto CreateObserver() {
				return CreateOfferV3Observer();
			}
		};
	}

#define TRAITS_BASED_TEST(TEST_NAME) \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(); \
	TEST(TEST_CLASS, TEST_NAME##_v1) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<ObserverV1Traits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_v2) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<ObserverV2Traits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_v3) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<ObserverV3Traits>(); } \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)()

	TRAITS_BASED_TEST(Offer_Commit) {
		// Arrange:
		auto values = CreateOfferValues();
		auto pNotification = CreateNotification<TTraits::Version>(values);

		// Assert:
		RunTest<TTraits>(NotifyMode::Commit, *pNotification, true, values);
	}

	TRAITS_BASED_TEST(Offer_Rollback) {
		// Arrange:
		auto values = CreateOfferValues();
		auto pNotification = CreateNotification<TTraits::Version>(values);

		// Assert:
		RunTest<TTraits>(NotifyMode::Rollback, *pNotification, false, values);
	}
}}
