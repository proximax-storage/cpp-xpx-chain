/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/cache_core/AccountStateCache.h"
#include "src/observers/Observers.h"
#include "tests/test/ExchangeTestUtils.h"
#include "tests/test/plugins/ObserverTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace observers {

#define TEST_CLASS RemoveOfferObserverTests

	DEFINE_COMMON_OBSERVER_TESTS(RemoveOfferV1, MosaicId())
	DEFINE_COMMON_OBSERVER_TESTS(RemoveOfferV2, MosaicId())

	namespace {
		using ObserverTestContext = test::ObserverTestContextT<test::ExchangeCacheFactory>;

		constexpr MosaicId Currency_Mosaic_Id(1234);

		struct RemoveOfferValues {
		public:
			explicit RemoveOfferValues(
					Height&& initialExpiryHeight,
					Height&& expectedExpiryHeight,
					std::vector<model::OfferMosaic>&& offersToRemove,
					state::ExchangeEntry&& initialEntry,
					state::ExchangeEntry&& expectedEntry)
				: InitialExpiryHeight(std::move(initialExpiryHeight))
				, ExpectedExpiryHeight(std::move(expectedExpiryHeight))
				, OffersToRemove(std::move(offersToRemove))
				, InitialEntry(std::move(initialEntry))
				, ExpectedEntry(std::move(expectedEntry))
			{}

		public:
			Height InitialExpiryHeight;
			Height ExpectedExpiryHeight;
			std::vector<model::OfferMosaic> OffersToRemove;
			state::ExchangeEntry InitialEntry;
			state::ExchangeEntry ExpectedEntry;
		};

		template<VersionType Version>
		std::unique_ptr<model::RemoveOfferNotification<Version>> CreateNotification(const RemoveOfferValues& values) {
			return std::make_unique<model::RemoveOfferNotification<Version>>(
				values.InitialEntry.owner(),
				values.OffersToRemove.size(),
				values.OffersToRemove.data());
		}

		void PrepareEntries(state::ExchangeEntry& entry1, state::ExchangeEntry& entry2) {
			entry1.buyOffers().emplace(MosaicId(1),
				state::BuyOffer{state::OfferBase{Amount(10), Amount(10), Amount(100), Height(15)}, Amount(100)});
			entry1.sellOffers().emplace(MosaicId(2),
				state::SellOffer{state::OfferBase{Amount(20), Amount(20), Amount(200), Height(5)}});

			entry2.expiredBuyOffers().emplace(Height(1), entry1.buyOffers());
			entry2.expiredSellOffers().emplace(Height(1), entry1.sellOffers());
		}

		auto CreateRemoveOfferValues(state::ExchangeEntry&& initialEntry, state::ExchangeEntry&& expectedEntry) {
			return RemoveOfferValues(
				Height(5),
				Height(0),
				{
					model::OfferMosaic{test::UnresolveXor(MosaicId(1)), model::OfferType::Buy},
					model::OfferMosaic{test::UnresolveXor(MosaicId(2)), model::OfferType::Sell},
				},
				std::move(initialEntry),
				std::move(expectedEntry));
		}

		template<typename TTraits>
		void RunTest(NotifyMode mode, const RemoveOfferValues& values, std::initializer_list<Height> expiryHeights) {
			// Arrange:
			Height currentHeight(1);
			ObserverTestContext context(mode, currentHeight);
			auto pNotification = CreateNotification<TTraits::Version>(values);
			auto pObserver = TTraits::CreateObserver();
			auto& accountCache = context.cache().sub<cache::AccountStateCache>();
			auto& exchangeCache = context.cache().sub<cache::ExchangeCache>();
			auto owner = values.InitialEntry.owner();

			// Populate cache.
			accountCache.addAccount(owner, currentHeight);
			if (NotifyMode::Rollback == mode) {
				auto accountIter = accountCache.find(owner);
				auto& accountEntry = accountIter.get();
				accountEntry.Balances.credit(Currency_Mosaic_Id, Amount(100), currentHeight);
				accountEntry.Balances.credit(MosaicId(2), Amount(20), currentHeight);
			}
			exchangeCache.insert(values.InitialEntry);

			// Act:
			test::ObserveNotification(*pObserver, *pNotification, context);

			// Assert: check the cache
			auto exchangeIter = exchangeCache.find(owner);
			const auto& exchangeEntry = exchangeIter.get();
			test::AssertEqualExchangeData(exchangeEntry, values.ExpectedEntry);
			EXPECT_EQ(values.InitialEntry.minExpiryHeight(), (NotifyMode::Commit == mode) ? values.InitialExpiryHeight : values.ExpectedExpiryHeight);
			EXPECT_EQ(exchangeEntry.minExpiryHeight(), (NotifyMode::Commit == mode) ? values.ExpectedExpiryHeight : values.InitialExpiryHeight);
			for (const auto& expiryHeight : expiryHeights) {
				auto keys = exchangeCache.expiringOfferOwners(expiryHeight);
				EXPECT_EQ(1, keys.size());
				EXPECT_EQ(owner, *keys.begin());
			}

			auto accountIter = accountCache.find(owner);
			const auto& accountEntry = accountIter.get();
			EXPECT_EQ(Amount(0), accountEntry.Balances.get(MosaicId(1)));
			EXPECT_EQ((NotifyMode::Commit == mode) ? Amount(100) : Amount(0), accountEntry.Balances.get(Currency_Mosaic_Id));
			EXPECT_EQ((NotifyMode::Commit == mode) ? Amount(20) : Amount(0), accountEntry.Balances.get(MosaicId(2)));
		}
	}

	struct RemoveOfferObserverV1Traits {
		static constexpr VersionType Version = 1;
		static auto CreateObserver() {
			return CreateRemoveOfferV1Observer(Currency_Mosaic_Id);
		}
	};

	struct RemoveOfferObserverV2Traits {
		static constexpr VersionType Version = 2;
		static auto CreateObserver() {
			return CreateRemoveOfferV2Observer(Currency_Mosaic_Id);
		}
	};

#define TRAITS_BASED_TEST(TEST_NAME) \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(); \
	TEST(TEST_CLASS, TEST_NAME##_v1) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<RemoveOfferObserverV1Traits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_v2) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<RemoveOfferObserverV2Traits>(); } \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)()

	TRAITS_BASED_TEST(RemoveOffer_Commit) {
		// Arrange:
		auto offerOwner = test::GenerateRandomByteArray<Key>();
		state::ExchangeEntry initialEntry(offerOwner);
		state::ExchangeEntry expectedEntry(offerOwner);
		PrepareEntries(initialEntry, expectedEntry);
		auto values = CreateRemoveOfferValues(std::move(initialEntry), std::move(expectedEntry));

		// Assert:
		RunTest<TTraits>(NotifyMode::Commit, values, {});
	}

	TRAITS_BASED_TEST(RemoveOffer_Rollback) {
		// Arrange:
		auto offerOwner = test::GenerateRandomByteArray<Key>();
		state::ExchangeEntry initialEntry(offerOwner);
		state::ExchangeEntry expectedEntry(offerOwner);
		PrepareEntries(expectedEntry, initialEntry);
		auto values = CreateRemoveOfferValues(std::move(initialEntry), std::move(expectedEntry));

		// Assert:
		RunTest<TTraits>(NotifyMode::Rollback, values, { Height(5) });
	}
}}
