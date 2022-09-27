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
#include "plugins/txes/liquidityprovider/tests/test/LiquidityProviderTestUtils.h"

namespace catapult { namespace observers {

#define TEST_CLASS DebitMosaicObserverTests

	DEFINE_COMMON_OBSERVER_TESTS(DebitMosaic, LiquidityProviderExchangeObserverImpl())

	namespace {
		using ObserverTestContext = test::ObserverTestContextT<test::LiquidityProviderCacheFactory>;
		using Notification = model::DebitMosaicNotification<1>;

		auto CreateConfig() {
			test::MutableBlockchainConfiguration config;
			config.Immutable.CurrencyMosaicId = test::GenerateRandomValue<MosaicId>();

			auto lpConfig = config::LiquidityProviderConfiguration::Uninitialized();

			lpConfig.PercentsDigitsAfterDot = 2;

			config.Network.SetPluginConfiguration(lpConfig);

			return config.ToConst();
		}

		struct LiquidityProviderInfo {
			state::LiquidityProviderEntry entry;
			Amount currencyAmount;
			Amount mosaicAmount;
		};

		LiquidityProviderInfo CreateInitialLInfo() {
			state::LiquidityProviderEntry entry(UnresolvedMosaicId {test::Random()});

			entry.setAdditionallyMinted(Amount {test::Random() / 4});
			entry.setBeta(500); // 5% fee
			return {entry, test::GenerateRandomValue<Amount>(), Amount {test::Random() / 4}};
		}

		struct CacheValues {
		public:
			CacheValues(const LiquidityProviderInfo& initialEntry)
						: InitialEntry(initialEntry)
						{}

		public:
			LiquidityProviderInfo InitialEntry;
			Height BlockHeight;
		};

		void RunTest(NotifyMode mode,
					 const CacheValues& values,
					 const Key& from,
					 const Key& to,
					 const Amount& mosaicAmount) {
			// Arrange:
			ObserverTestContext context(mode, values.BlockHeight, CreateConfig());
			Notification notification(from, to, values.InitialEntry.entry.mosaicId(), mosaicAmount);

			auto liquidityProvider = std::make_shared<observers::LiquidityProviderExchangeObserverImpl>();

			auto pObserver = CreateDebitMosaicObserver(*liquidityProvider);
			auto& lpCache = context.cache().sub<cache::LiquidityProviderCache>();
			auto& accountCache = context.cache().sub<cache::AccountStateCache>();

			auto currencyId = MosaicId{context.observerContext().Config.Immutable.CurrencyMosaicId};

			const auto& info = values.InitialEntry;
			const auto& initialEntry = info.entry;
			lpCache.insert(info.entry);

			auto mosaicId = context.observerContext().Resolvers.resolve(initialEntry.mosaicId());

			test::AddAccountState(accountCache, from, Height(1), { { mosaicId, mosaicAmount } });
			test::AddAccountState(accountCache, to);

			test::AddAccountState(
					accountCache,
					initialEntry.providerKey(),
					Height(1),
					{ { currencyId, info.currencyAmount }, { mosaicId, info.mosaicAmount } });

			// Act:
			test::ObserveNotification(*pObserver, notification, context);

			// Assert: check the cache
			auto actualEntryIter = lpCache.find(initialEntry.mosaicId());
			const auto& actualEntry = actualEntryIter.get();

			auto fromBalance = accountCache.find(from).get().Balances.get(mosaicId);
			ASSERT_EQ(fromBalance, Amount{0});

			ASSERT_EQ(actualEntry.additionallyMinted(), initialEntry.additionallyMinted() - mosaicAmount);

			auto toBalance = accountCache.find(to).get().Balances.get(currencyId);
			auto lpCurrencyBalance = accountCache.find(actualEntry.providerKey()).get().Balances.get(currencyId);

			ASSERT_EQ(info.currencyAmount - lpCurrencyBalance, toBalance);

			auto lpMosaicBalance = accountCache.find(actualEntry.providerKey()).get().Balances.get(mosaicId);
			state::ExchangeRate initialRate = {info.currencyAmount, info.mosaicAmount + info.entry.additionallyMinted()};
			state::ExchangeRate actualRate = {lpCurrencyBalance, lpMosaicBalance + actualEntry.additionallyMinted()};

			ASSERT_LT(initialRate, actualRate);

			ASSERT_EQ(actualEntry.additionallyMinted(), initialEntry.additionallyMinted() - mosaicAmount);
		}
	}

	TEST(TEST_CLASS, DebitMosaicObserver_Success) {
		for (int i = 0; i < 1000; i++) {
			CacheValues values(CreateInitialLInfo());

			Amount toTransfer =
					Amount { test::RandomInRange(0UL, values.InitialEntry.entry.additionallyMinted().unwrap()) };

			RunTest(NotifyMode::Commit,
					values,
					test::GenerateRandomByteArray<Key>(),
					test::GenerateRandomByteArray<Key>(),
					toTransfer);
		}
	}

	TEST(TEST_CLASS, DebitMosaicObserver_Rollback) {
		// Arrange:
		CacheValues values(CreateInitialLInfo());

		// Assert
		EXPECT_THROW(RunTest(NotifyMode::Rollback, values, test::GenerateRandomByteArray<Key>(), test::GenerateRandomByteArray<Key>(), test::GenerateRandomValue<Amount>()), catapult_runtime_error);
	}
}}