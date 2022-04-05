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
#include "tests/utils/LiquidityProviderTestUtils.h"

namespace catapult { namespace observers {

#define TEST_CLASS CreateLiquidityProviderObserverTests

	DEFINE_COMMON_OBSERVER_TESTS(CreateLiquidityProvider, )

	namespace {
		using ObserverTestContext = test::ObserverTestContextT<test::LiquidityProviderCacheFactory>;
		using Notification = model::CreateLiquidityProviderNotification<1>;

		auto CreateConfig() {
			test::MutableBlockchainConfiguration config;
			config.Immutable.CurrencyMosaicId = test::GenerateRandomValue<MosaicId>();

			auto lpConfig = config::LiquidityProviderConfiguration::Uninitialized();

			lpConfig.PercentsDigitsAfterDot = 2;

			config.Network.SetPluginConfiguration(lpConfig);

			return config.ToConst();
		}

		void RunTest(NotifyMode mode,
					 const Key& providerKey,
					 const Key& owner,
					 const UnresolvedMosaicId& providerMosaicId,
					 const Amount& currencyDeposit,
					 const Amount& initialMosaicsMinting,
					 uint32_t slashingPeriod,
					 uint16_t windowSize,
					 const Key& slashingAccount,
					 uint32_t alpha,
					 uint32_t beta) {
			// Arrange:
			ObserverTestContext context(mode, Height(0), CreateConfig());
			Notification notification(
					providerKey,
					owner,
					providerMosaicId,
					currencyDeposit,
					initialMosaicsMinting,
					slashingPeriod,
					windowSize,
					slashingAccount,
					alpha,
					beta);

			auto pObserver = CreateCreateLiquidityProviderObserver();

			// Act:
			test::ObserveNotification(*pObserver, notification, context);

			// Assert: check the cache
			auto& lpCache = context.cache().sub<cache::LiquidityProviderCache>();
			const auto& actualEntry = lpCache.find(providerMosaicId).get();

			ASSERT_EQ(actualEntry.providerKey(), providerKey);
			ASSERT_EQ(actualEntry.owner(), owner);
			ASSERT_EQ(actualEntry.slashingPeriod(), slashingPeriod);
			ASSERT_EQ(actualEntry.windowSize(), windowSize);
			ASSERT_EQ(actualEntry.slashingAccount(), slashingAccount);
			ASSERT_EQ(actualEntry.alpha(), alpha);
			ASSERT_EQ(actualEntry.beta(), beta);
			ASSERT_TRUE(actualEntry.turnoverHistory().empty());

			const auto& actualRecentTurnover = actualEntry.recentTurnover();
			ASSERT_EQ(actualRecentTurnover.m_turnover, Amount{0});
			ASSERT_EQ(actualRecentTurnover.m_rate.m_mosaicAmount, initialMosaicsMinting);
			ASSERT_EQ(actualRecentTurnover.m_rate.m_currencyAmount, currencyDeposit);
		}
	}

	TEST(TEST_CLASS, CreateLiquidityProviderObserver_Success) {
		// Assert
		RunTest(NotifyMode::Commit,
				test::GenerateRandomByteArray<Key>(),
				test::GenerateRandomByteArray<Key>(),
				test::GenerateRandomValue<UnresolvedMosaicId>(),
				test::GenerateRandomValue<Amount>(),
				test::GenerateRandomValue<Amount>(),
				test::Random16(),
				test::Random16(),
				test::GenerateRandomByteArray<Key>(),
				test::Random16(),
				test::Random16());
	}

	TEST(TEST_CLASS, CreateLiquidityProviderObserver_Rollback) {
		// Arrange:

		// Assert
		EXPECT_THROW(
				RunTest(NotifyMode::Commit,
						test::GenerateRandomByteArray<Key>(),
						test::GenerateRandomByteArray<Key>(),
						test::GenerateRandomValue<UnresolvedMosaicId>(),
						test::GenerateRandomValue<Amount>(),
						test::GenerateRandomValue<Amount>(),
						test::Random16(),
						test::Random16(),
						test::GenerateRandomByteArray<Key>(),
						test::Random16(),
						test::Random16()),
				catapult_runtime_error);
	}
}}