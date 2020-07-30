/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/cache_core/AccountStateCache.h"
#include "catapult/model/Address.h"
#include "src/observers/Observers.h"
#include "tests/test/ExchangeTestUtils.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "tests/test/core/NotificationTestUtils.h"
#include "tests/test/other/MutableBlockchainConfiguration.h"
#include "tests/test/plugins/ObserverTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace observers {

#define TEST_CLASS CleanupOffersObserverTests

	DEFINE_COMMON_OBSERVER_TESTS(CleanupOffers,)

	namespace {
		using ObserverTestContext = test::ObserverTestContextT<test::ExchangeCacheFactory>;
		using Notification = model::BlockNotification<1>;

		constexpr MosaicId Currency_Mosaic_Id(1234);
		constexpr uint32_t Max_Rollback_Blocks(50);
		constexpr Height Current_Height(55);
		constexpr auto Network_Identifier = model::NetworkIdentifier::Mijin_Test;

		auto CreateConfig() {
			test::MutableBlockchainConfiguration config;
			config.Immutable.CurrencyMosaicId = Currency_Mosaic_Id;
			config.Network.MaxRollbackBlocks = Max_Rollback_Blocks;
			return config.ToConst();
		}

		struct CacheValues {
		public:
			explicit CacheValues(
					std::vector<state::ExchangeEntry>&& initialEntries,
					std::vector<state::ExchangeEntry>&& expectedEntries,
					std::vector<state::AccountState>&& initialAccounts,
					std::vector<state::AccountState>&& expectedAccounts)
				: InitialEntries(std::move(initialEntries))
				, ExpectedEntries(std::move(expectedEntries))
				, InitialAccounts(std::move(initialAccounts))
				, ExpectedAccounts(std::move(expectedAccounts))
			{}

		public:
			std::vector<state::ExchangeEntry> InitialEntries;
			std::vector<state::ExchangeEntry> ExpectedEntries;
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
				NotifyMode mode,
				std::vector<state::ExchangeEntry>& initialEntries,
				std::vector<state::ExchangeEntry>& expectedEntries,
				std::vector<state::AccountState>& initialAccounts,
				std::vector<state::AccountState>& expectedAccounts) {
			state::BuyOffer buyOffer{state::OfferBase{Amount(10), Amount(10), Amount(100), Current_Height + Height(5)}, Amount(100)};
			state::SellOffer sellOffer{state::OfferBase{Amount(20), Amount(20), Amount(200), Current_Height}};
			Height pruneHeight(Current_Height.unwrap() - Max_Rollback_Blocks);

			// Sell offer expired.
			state::ExchangeEntry initialEntry1(test::GenerateRandomByteArray<Key>());
			initialEntry1.buyOffers().emplace(MosaicId(1), buyOffer);
			initialEntry1.sellOffers().emplace(MosaicId(2), sellOffer);
			initialEntries.push_back(initialEntry1);

			state::ExchangeEntry expectedEntry1(initialEntry1.owner());
			expectedEntry1.buyOffers().emplace(MosaicId(1), buyOffer);
			expectedEntry1.expiredSellOffers()[Current_Height].emplace(MosaicId(2), sellOffer);
			expectedEntries.push_back(expectedEntry1);

			initialAccounts.push_back(CreateAccount(initialEntry1.owner()));
			expectedAccounts.push_back(CreateAccount(initialEntry1.owner()));
			expectedAccounts.back().Balances.credit(MosaicId(2), Amount(20));

			// Buy offer expired.
			state::ExchangeEntry initialEntry2(test::GenerateRandomByteArray<Key>());
			buyOffer.Deadline = Current_Height;
			initialEntry2.buyOffers().emplace(MosaicId(1), buyOffer);
			sellOffer.Deadline = Current_Height + Height(5);
			initialEntry2.sellOffers().emplace(MosaicId(2), sellOffer);
			initialEntries.push_back(initialEntry2);

			state::ExchangeEntry expectedEntry2(initialEntry2.owner());
			expectedEntry2.expiredBuyOffers()[Current_Height].emplace(MosaicId(1), buyOffer);
			expectedEntry2.sellOffers().emplace(MosaicId(2), sellOffer);
			expectedEntries.push_back(expectedEntry2);

			initialAccounts.push_back(CreateAccount(initialEntry2.owner()));
			expectedAccounts.push_back(CreateAccount(initialEntry2.owner()));
			expectedAccounts.back().Balances.credit(Currency_Mosaic_Id, Amount(100));

			// All offers expired.
			state::ExchangeEntry initialEntry3(test::GenerateRandomByteArray<Key>());
			initialEntry3.buyOffers().emplace(MosaicId(1), buyOffer);
			sellOffer.Deadline = Current_Height;
			initialEntry3.sellOffers().emplace(MosaicId(2), sellOffer);
			initialEntries.push_back(initialEntry3);

			state::ExchangeEntry expectedEntry3(initialEntry3.owner());
			expectedEntry3.expiredBuyOffers()[Current_Height].emplace(MosaicId(1), buyOffer);
			expectedEntry3.expiredSellOffers()[Current_Height].emplace(MosaicId(2), sellOffer);
			expectedEntries.push_back(expectedEntry3);

			initialAccounts.push_back(CreateAccount(initialEntry3.owner()));
			expectedAccounts.push_back(CreateAccount(initialEntry3.owner()));
			expectedAccounts.back().Balances.credit(Currency_Mosaic_Id, Amount(100));
			expectedAccounts.back().Balances.credit(MosaicId(2), Amount(20));

			// Sell offer pruned.
			state::ExchangeEntry initialEntry4(test::GenerateRandomByteArray<Key>());
			initialEntry4.expiredBuyOffers()[pruneHeight + Height(1)].emplace(MosaicId(1), buyOffer);
			if (NotifyMode::Commit == mode)
				initialEntry4.expiredSellOffers()[pruneHeight].emplace(MosaicId(2), sellOffer);
			initialEntries.push_back(initialEntry4);

			state::ExchangeEntry expectedEntry4(initialEntry4.owner());
			expectedEntry4.expiredBuyOffers()[pruneHeight + Height(1)].emplace(MosaicId(1), buyOffer);
			expectedEntries.push_back(expectedEntry4);

			// Buy offer pruned.
			state::ExchangeEntry initialEntry5(test::GenerateRandomByteArray<Key>());
			if (NotifyMode::Commit == mode)
				initialEntry5.expiredBuyOffers()[pruneHeight].emplace(MosaicId(1), buyOffer);
			initialEntry5.expiredSellOffers()[pruneHeight + Height(1)].emplace(MosaicId(2), sellOffer);
			initialEntries.push_back(initialEntry5);

			state::ExchangeEntry expectedEntry5(initialEntry5.owner());
			expectedEntry5.expiredSellOffers()[pruneHeight + Height(1)].emplace(MosaicId(2), sellOffer);
			expectedEntries.push_back(expectedEntry5);

			if (NotifyMode::Commit == mode) {
				// All offers pruned along with the entry.
				state::ExchangeEntry initialEntry6(test::GenerateRandomByteArray<Key>());
				initialEntry6.expiredBuyOffers()[pruneHeight].emplace(MosaicId(1), buyOffer);
				initialEntry6.expiredSellOffers()[pruneHeight].emplace(MosaicId(2), sellOffer);
				initialEntries.push_back(initialEntry6);
			}
		}

		void RunTest(NotifyMode mode, const CacheValues& values) {
			// Arrange:
			ObserverTestContext context(mode, Current_Height, CreateConfig());
			model::BlockNotification<1> notification = test::CreateBlockNotification();
			auto pObserver = CreateCleanupOffersObserver();
			auto& exchangeCache = context.cache().sub<cache::ExchangeCache>();
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
				test::AssertEqualExchangeData(actualEntry, expectedEntry);
			}

			for (auto i = values.ExpectedEntries.size(); i < values.InitialEntries.size(); ++i) {
				auto iter = exchangeCache.find(values.InitialEntries[i].owner());
				const auto& entry = iter.get();
				test::AssertEqualExchangeData(state::ExchangeEntry(entry.owner()), entry);
			}

			for (const auto& expectedAccount : values.ExpectedAccounts) {
				auto iter = accountCache.find(expectedAccount.Address);
				const auto& actualAccount = iter.get();
				AssertAccount(expectedAccount, actualAccount);
			}
		}
	}

	TEST(TEST_CLASS, CleanupOffers_Commit) {
		// Arrange:
		std::vector<state::ExchangeEntry> initialEntries;
		std::vector<state::ExchangeEntry> expectedEntries;
		std::vector<state::AccountState> initialAccounts;
		std::vector<state::AccountState> expectedAccounts;
		PrepareEntries(NotifyMode::Commit, initialEntries, expectedEntries, initialAccounts, expectedAccounts);
		CacheValues values(std::move(initialEntries), std::move(expectedEntries), std::move(initialAccounts), std::move(expectedAccounts));

		// Assert:
		RunTest(NotifyMode::Commit, values);
	}

	TEST(TEST_CLASS, CleanupOffers_Rollback) {
		// Arrange:
		std::vector<state::ExchangeEntry> initialEntries;
		std::vector<state::ExchangeEntry> expectedEntries;
		std::vector<state::AccountState> initialAccounts;
		std::vector<state::AccountState> expectedAccounts;
		PrepareEntries(NotifyMode::Rollback, expectedEntries, initialEntries, expectedAccounts, initialAccounts);
		CacheValues values(std::move(initialEntries), std::move(expectedEntries), std::move(initialAccounts), std::move(expectedAccounts));

		// Assert:
		RunTest(NotifyMode::Rollback, values);
	}
}}
