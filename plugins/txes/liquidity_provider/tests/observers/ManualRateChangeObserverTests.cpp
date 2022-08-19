/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <src/cache/LiquidityProviderCache.h>
#include <src/observers/LiquidityProviderExchangeObserverImpl.h>
#include "src/observers/Observers.h"
#include "tests/test/plugins/ObserverTestUtils.h"
#include "tests/TestHarness.h"
#include "src/config/LiquidityProviderConfiguration.h"
#include "plugins/txes/liquidity_provider/tests/test/LiquidityProviderTestUtils.h"

namespace catapult { namespace observers {

#define TEST_CLASS ManualRateChangeObserverTests

	DEFINE_COMMON_OBSERVER_TESTS(ManualRateChange,)

	namespace {
		using ObserverTestContext = test::ObserverTestContextT<test::LiquidityProviderCacheFactory>;
		using Notification = model::ManualRateChangeNotification<1>;

		auto CreateConfig() {
			test::MutableBlockchainConfiguration config;
			config.Immutable.CurrencyMosaicId = test::GenerateRandomValue<MosaicId>();

			auto lpConfig = config::LiquidityProviderConfiguration::Uninitialized();

			lpConfig.PercentsDigitsAfterDot = 2;

			config.Network.SetPluginConfiguration(lpConfig);

			return config.ToConst();
		}

		state::LiquidityProviderEntry CreateInitialLInfo() {
			state::LiquidityProviderEntry entry(UnresolvedMosaicId {test::Random()});

			auto maxWindowSize = test::Random16();
			entry.setWindowSize(maxWindowSize);

			for (int i = 0; i < maxWindowSize; i++) {
				entry.turnoverHistory().push_back(state::HistoryObservation{
					state::ExchangeRate{
						Amount {test::Random()},
						Amount {test::Random()}
						},
						Amount {test::Random()}});
			}

			entry.recentTurnover() = state::HistoryObservation{
				state::ExchangeRate{
					Amount {test::Random()},
					Amount {test::Random()}
					},
					Amount {test::Random()}};
			entry.setCreationHeight(Height{ 0 });
			entry.setSlashingAccount(test::GenerateRandomByteArray<Key>());
			entry.setProviderKey(test::GenerateRandomByteArray<Key>());
			entry.setAdditionallyMinted(Amount {test::Random() / 4});

			return entry;
		}

		struct CacheValues {
		public:
			CacheValues(const state::LiquidityProviderEntry& initialEntry,
						const Amount& currencyBalance,
						const Amount& mosaicBalance)
						: InitialEntry(initialEntry)
						, CurrencyBalance(currencyBalance)
						, MosaicBalance(mosaicBalance)
						{}

		public:
			state::LiquidityProviderEntry InitialEntry;
			Amount CurrencyBalance;
			Amount MosaicBalance;
		};

		void RunTest(NotifyMode mode,
					 const CacheValues& values,
					 bool currencyBalanceIncrease,
					 const Amount& currencyBalanceChange,
					 bool mosaicBalanceIncrease,
					 const Amount& mosaicBalanceChange) {
			// Arrange:
			auto height = Height(1000);
			ObserverTestContext context(mode, height, CreateConfig());

			const auto& initialEntry = values.InitialEntry;
			Notification notification(
					initialEntry.owner(),
					initialEntry.mosaicId(),
					currencyBalanceIncrease,
					currencyBalanceChange,
					mosaicBalanceIncrease,
					mosaicBalanceChange);

			auto pObserver = CreateManualRateChangeObserver();
			auto& lpCache = context.cache().sub<cache::LiquidityProviderCache>();
			auto& accountCache = context.cache().sub<cache::AccountStateCache>();

			auto currencyId = MosaicId { context.observerContext().Config.Immutable.CurrencyMosaicId };

			lpCache.insert(values.InitialEntry);

			auto mosaicId = context.observerContext().Resolvers.resolve(initialEntry.mosaicId());

			const Amount fromInitialCurrencyBalance((uint64_t)ULLONG_MAX);
			test::AddAccountState(accountCache, initialEntry.owner(), Height(1), { { currencyId, fromInitialCurrencyBalance } });
			test::AddAccountState(accountCache, initialEntry.slashingAccount());

			test::AddAccountState(
					accountCache,
					initialEntry.providerKey(),
					Height(1),
					{ { currencyId, values.CurrencyBalance }, { mosaicId, values.MosaicBalance } });

			// Act:
			test::ObserveNotification(*pObserver, notification, context);

			// Assert: check the cache
			auto actualEntryIter = lpCache.find(initialEntry.mosaicId());
			const auto& actualEntry = actualEntryIter.get();

			auto lpAccountStateIter = accountCache.find(actualEntry.providerKey());
			const auto& lpAccountState = lpAccountStateIter.get();
			auto lpCurrencyBalance = lpAccountState.Balances.get(currencyId);
			auto lpMosaicBalance = lpAccountState.Balances.get(mosaicId);

			auto ownerCurrencyBalance = accountCache.find(actualEntry.owner()).get().Balances.get(currencyId);
			auto slashingAccountCurrencyBalance = accountCache.find(actualEntry.slashingAccount()).get().Balances.get(currencyId);

			if (notification.CurrencyBalanceIncrease) {
				ASSERT_EQ(lpCurrencyBalance, values.CurrencyBalance + currencyBalanceChange);
 				ASSERT_EQ(ownerCurrencyBalance, fromInitialCurrencyBalance - currencyBalanceChange);
			}
			else {
				ASSERT_EQ(lpCurrencyBalance, values.CurrencyBalance - currencyBalanceChange);
				ASSERT_EQ(slashingAccountCurrencyBalance, currencyBalanceChange);
			}

			if (notification.MosaicBalanceIncrease) {
				ASSERT_EQ(lpMosaicBalance, values.MosaicBalance + mosaicBalanceChange);
			}
			else {
				ASSERT_EQ(lpMosaicBalance, values.MosaicBalance - mosaicBalanceChange);
			}

			ASSERT_EQ(actualEntry.creationHeight(), height);
			ASSERT_TRUE(actualEntry.turnoverHistory().empty());
			const auto& actualRecentTurnover = actualEntry.recentTurnover();
			ASSERT_EQ(actualRecentTurnover.m_turnover, Amount(0));

			ASSERT_EQ(actualRecentTurnover.m_rate.m_currencyAmount, lpCurrencyBalance);
			ASSERT_EQ(actualRecentTurnover.m_rate.m_mosaicAmount, lpMosaicBalance + actualEntry.additionallyMinted());
			ASSERT_EQ(actualEntry.additionallyMinted(), initialEntry.additionallyMinted());
		}
	}

	TEST(TEST_CLASS, DebitMosaicObserver_Success) {
		// Arrange:
		CacheValues values(CreateInitialLInfo(), Amount {test::Random() / 16}, Amount {test::Random() / 16});

		// Assert
		for (int i = 0; i < 10000; i++) {
			RunTest(NotifyMode::Commit,
					values,
					test::RandomByte() % 2,
					Amount{test::RandomInRange(0UL, values.CurrencyBalance.unwrap())},
					test::RandomByte() % 2,
					Amount{test::RandomInRange(0UL, values.MosaicBalance.unwrap())});
		}
	}

//	TEST(TEST_CLASS, DebitMosaicObserver_Rollback) {
//		// Arrange:
//		CacheValues values(CreateInitialLInfo());
//
//		// Assert
//		EXPECT_THROW(RunTest(NotifyMode::Rollback, values, test::GenerateRandomByteArray<Key>(), test::GenerateRandomByteArray<Key>(), test::GenerateRandomValue<Amount>()), catapult_runtime_error);
//	}
}}