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

#define TEST_CLASS ExchangeObserverTests

	DEFINE_COMMON_OBSERVER_TESTS(ExchangeV1, )
	DEFINE_COMMON_OBSERVER_TESTS(ExchangeV2, )

	namespace {
		using ObserverTestContext = test::ObserverTestContextT<test::ExchangeCacheFactory>;

		struct ExchangeValues {
		public:
			explicit ExchangeValues(
					Height&& initialExpiryHeight,
					Height&& expectedExpiryHeight,
					std::vector<model::MatchedOffer>&& matchedOffers,
					state::ExchangeEntry&& initialEntry,
					state::ExchangeEntry&& expectedEntry)
				: InitialExpiryHeight(std::move(initialExpiryHeight))
				, ExpectedExpiryHeight(std::move(expectedExpiryHeight))
				, MatchedOffers(std::move(matchedOffers))
				, InitialEntry(std::move(initialEntry))
				, ExpectedEntry(std::move(expectedEntry))
			{}

		public:
			Height InitialExpiryHeight;
			Height ExpectedExpiryHeight;
			std::vector<model::MatchedOffer> MatchedOffers;
			state::ExchangeEntry InitialEntry;
			state::ExchangeEntry ExpectedEntry;
		};

		template<typename Notification>
		std::unique_ptr<Notification> CreateNotification(const ExchangeValues& values) {
			return std::make_unique<Notification>(
				values.InitialEntry.owner(),
				values.MatchedOffers.size(),
				values.MatchedOffers.data());
		}

		void PrepareEntries(state::ExchangeEntry& entry1, state::ExchangeEntry& entry2) {
			entry1.buyOffers().emplace(MosaicId(1),
				state::BuyOffer{state::OfferBase{Amount(10), Amount(10), Amount(100), Height(15)}, Amount(100)});
			entry1.sellOffers().emplace(MosaicId(2),
				state::SellOffer{state::OfferBase{Amount(20), Amount(20), Amount(150), Height(5)}});

			entry2.buyOffers().emplace(MosaicId(1),
				state::BuyOffer{state::OfferBase{Amount(5), Amount(10), Amount(100), Height(15)}, Amount(50)});
			auto pair = entry2.expiredSellOffers().emplace(Height(1), entry1.sellOffers());
			pair.first->second.at(MosaicId(2)).Amount = Amount(0);
		}

		auto CreateExchangeValues(state::ExchangeEntry&& initialEntry, state::ExchangeEntry&& expectedEntry, const Key& offerOwner) {
			return ExchangeValues(
				Height(5),
				Height(15),
				{
					model::MatchedOffer{model::Offer{{test::UnresolveXor(MosaicId(1)), Amount(5)}, Amount(50), model::OfferType::Buy}, offerOwner},
					model::MatchedOffer{model::Offer{{test::UnresolveXor(MosaicId(2)), Amount(20)}, Amount(150), model::OfferType::Sell}, offerOwner},
				},
				std::move(initialEntry),
				std::move(expectedEntry));
		}

		template<typename TTraits>
		void RunTest(NotifyMode mode, const ExchangeValues& values, std::initializer_list<Height> expiryHeights) {
			// Arrange:
			Height currentHeight(1);
			ObserverTestContext context(mode, currentHeight);
			auto pNotification = CreateNotification<model::ExchangeNotification<TTraits::Version>>(values);
			auto pObserver = TTraits::CreateObserver();
			auto& delta = context.cache().sub<cache::ExchangeCache>();
			const auto& owner = values.InitialEntry.owner();

			// Populate cache.
			delta.insert(values.InitialEntry);

			// Act:
			test::ObserveNotification(*pObserver, *pNotification, context);

			// Assert: check the cache
			auto iter = delta.find(owner);
			const auto& entry = iter.get();
			test::AssertEqualExchangeData(entry, values.ExpectedEntry);
			EXPECT_EQ(values.InitialEntry.minExpiryHeight(), (NotifyMode::Commit == mode) ? values.InitialExpiryHeight : values.ExpectedExpiryHeight);
			EXPECT_EQ(entry.minExpiryHeight(), (NotifyMode::Commit == mode) ? values.ExpectedExpiryHeight : values.InitialExpiryHeight);
			for (const auto& expiryHeight : expiryHeights) {
				auto keys = delta.expiringOfferOwners(expiryHeight);
				EXPECT_EQ(1, keys.size());
				EXPECT_EQ(owner, *keys.begin());
			}
		}
	}

	struct ExchangeObserverV1Traits {
		static constexpr VersionType Version = 1;
		static auto CreateObserver() {
			return CreateExchangeV1Observer();
		}
	};

	struct ExchangeObserverV2Traits {
		static constexpr VersionType Version = 2;
		static auto CreateObserver() {
			return CreateExchangeV2Observer();
		}
	};

#define TRAITS_BASED_TEST(TEST_NAME) \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(); \
	TEST(TEST_CLASS, TEST_NAME##_v1) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<ExchangeObserverV1Traits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_v2) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<ExchangeObserverV2Traits>(); } \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)()

	TRAITS_BASED_TEST(Exchange_Commit) {
		// Arrange:
		auto offerOwner = test::GenerateRandomByteArray<Key>();
		state::ExchangeEntry initialEntry(offerOwner);
		state::ExchangeEntry expectedEntry(offerOwner);
		PrepareEntries(initialEntry, expectedEntry);
		auto values = CreateExchangeValues(std::move(initialEntry), std::move(expectedEntry), offerOwner);

		// Assert:
		RunTest<TTraits>(NotifyMode::Commit, values, { Height(1), Height(15) });
	}

	TRAITS_BASED_TEST(Exchange_Rollback) {
		// Arrange:
		auto offerOwner = test::GenerateRandomByteArray<Key>();
		state::ExchangeEntry initialEntry(offerOwner);
		state::ExchangeEntry expectedEntry(offerOwner);
		PrepareEntries(expectedEntry, initialEntry);
		auto values = CreateExchangeValues(std::move(initialEntry), std::move(expectedEntry), offerOwner);

		// Assert:
		RunTest<TTraits>(NotifyMode::Rollback, values, { Height(5) });
	}
}}
