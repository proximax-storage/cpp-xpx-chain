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

	DEFINE_COMMON_OBSERVER_TESTS(Offer, )

	namespace {
		using ObserverTestContext = test::ObserverTestContextT<test::ExchangeCacheFactory>;
		using Notification = model::OfferNotification<1>;

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

		std::unique_ptr<Notification> CreateNotification(OfferValues& values) {
			return std::make_unique<Notification>(
				values.Owner,
				values.Offers.size(),
				values.Offers.data());
		}

		Height GetOfferDeadline(const BlockDuration& duration, const Height& currentHeight) {
			auto maxDeadline = std::numeric_limits<Height::ValueType>::max();
			return Height((duration.unwrap() < maxDeadline - currentHeight.unwrap()) ? currentHeight.unwrap() + duration.unwrap() : maxDeadline);
		}

		void RunTest(
				NotifyMode mode,
				const Notification& notification,
				bool entryExists,
				const OfferValues& expectedValues) {
			// Arrange:
			Height currentHeight(1);
			ObserverTestContext context(mode, currentHeight);
			auto pObserver = CreateOfferObserver();
			auto& delta = context.cache().sub<cache::ExchangeCache>();

			if (NotifyMode::Rollback == mode) {
				// Populate cache.
				state::ExchangeEntry entry(notification.Owner);
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
	}

	TEST(TEST_CLASS, Offer_Commit) {
		// Arrange:
		auto values = CreateOfferValues();
		auto pNotification = CreateNotification(values);

		// Assert:
		RunTest(NotifyMode::Commit, *pNotification, true, values);
	}

	TEST(TEST_CLASS, Offer_Rollback) {
		// Arrange:
		auto values = CreateOfferValues();
		auto pNotification = CreateNotification(values);

		// Assert:
		RunTest(NotifyMode::Rollback, *pNotification, false, values);
	}
}}
