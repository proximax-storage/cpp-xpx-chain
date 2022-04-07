/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <src/cache/LiquidityProviderCache.h>
#include "src/observers/Observers.h"
#include "tests/test/plugins/ObserverTestUtils.h"
#include "tests/TestHarness.h"
#include "src/config/LiquidityProviderConfiguration.h"
#include "plugins/txes/liquidity_provider/tests/test/LiquidityProviderTestUtils.h"

namespace catapult { namespace observers {

#define TEST_CLASS SlashingObserverTests

	DEFINE_COMMON_OBSERVER_TESTS(Slashing, std::make_shared<cache::LiquidityProviderKeyCollector>())

	namespace {
		using ObserverTestContext = test::ObserverTestContextT<test::LiquidityProviderCacheFactory>;
		using Notification = model::BlockNotification<2>;

		auto CreateConfig() {
			test::MutableBlockchainConfiguration config;
			config.Immutable.CurrencyMosaicId = test::GenerateRandomValue<MosaicId>();

			auto lpConfig = config::LiquidityProviderConfiguration::Uninitialized();

			config.Network.SetPluginConfiguration(lpConfig);

			return config.ToConst();
		}

		struct LiquidityProviderInfo {
			state::LiquidityProviderEntry entry;
			Amount currencyAmount;
			Amount mosaicAmount;
		};

		LiquidityProviderInfo CreateInitialLInfo(uint32_t slashingPeriod) {
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
			entry.setSlashingPeriod(slashingPeriod);
			entry.setSlashingAccount(test::GenerateRandomByteArray<Key>());
			entry.setProviderKey(test::GenerateRandomByteArray<Key>());
			entry.setAdditionallyMinted(Amount {test::Random() / 4});

			return {entry, test::GenerateRandomValue<Amount>(), Amount {test::Random() / 4}};
		}

		struct CacheValues {
		public:
			CacheValues(const std::vector<LiquidityProviderInfo>& initialEntries,
						const Height& blockHeight)
						: InitialEntries(initialEntries)
						, BLockHeight(blockHeight)
						{}

		public:
			std::vector<LiquidityProviderInfo> InitialEntries;
			Height BLockHeight;
		};

		void RunTest(NotifyMode mode, const CacheValues& values) {
			// Arrange:
			ObserverTestContext context(mode, values.BLockHeight, CreateConfig());
			Notification notification(test::GenerateRandomByteArray<Hash256>(), test::GenerateRandomValue<Timestamp>());

			auto pKeyCollector = std::make_shared<cache::LiquidityProviderKeyCollector>();

			auto pObserver = CreateSlashingObserver(pKeyCollector);
			auto& lpCache = context.cache().sub<cache::LiquidityProviderCache>();
			auto& accountCache = context.cache().sub<cache::AccountStateCache>();

			auto currencyId = MosaicId{context.observerContext().Config.Immutable.CurrencyMosaicId};

			for (const auto& info: values.InitialEntries) {
				const auto& entry = info.entry;
				lpCache.insert(info.entry);
				test::AddAccountState(accountCache, entry.slashingAccount());

				pKeyCollector->keys().insert(entry.mosaicId());

				auto mosaicId = MosaicId{entry.mosaicId().unwrap()};

				test::AddAccountState(
						accountCache,
						entry.providerKey(),
						Height(1),
						{ { currencyId, info.currencyAmount }, { mosaicId, info.mosaicAmount } });
			}

			// Act:
			test::ObserveNotification(*pObserver, notification, context);

			// Assert: check the cache
			for (const auto& info: values.InitialEntries) {

				const auto& initialEntry = info.entry;

				const auto& initialTurnoverHistory = initialEntry.turnoverHistory();

				auto expectedMaxHistoryObservationIt = std::max_element(
						initialTurnoverHistory.begin(), initialTurnoverHistory.end(), [] (const auto& a, const auto& b) {
					return a.m_turnover < b.m_turnover;
				});

				const auto& actualLiquidityProviderEntry = lpCache.find(initialEntry.mosaicId()).get();
				const auto& actualRecentTurnover = actualLiquidityProviderEntry.recentTurnover();
				ASSERT_EQ(actualRecentTurnover.m_turnover, Amount{0});

				const auto& accountEntry = accountCache.find(initialEntry.providerKey()).get();

				auto resolvedMosaicId = context.observerContext().Resolvers.resolve(actualLiquidityProviderEntry.mosaicId());
				state::ExchangeRate actualRate = {accountEntry.Balances.get(currencyId),
												  accountEntry.Balances.get(resolvedMosaicId)
														   + actualLiquidityProviderEntry.additionallyMinted()};

				ASSERT_EQ(actualRate.m_currencyAmount, actualRecentTurnover.m_rate.m_currencyAmount);
				ASSERT_EQ(actualRate.m_mosaicAmount, actualRecentTurnover.m_rate.m_mosaicAmount);

				ASSERT_TRUE(actualRate.m_currencyAmount <= info.currencyAmount);

				state::ExchangeRate initialRate = {info.currencyAmount, info.mosaicAmount + initialEntry.additionallyMinted()};

				if (expectedMaxHistoryObservationIt->m_rate < initialRate) {
					ASSERT_TRUE(expectedMaxHistoryObservationIt->m_rate < actualRate);
				}

				auto currencySlashed = info.currencyAmount - actualRate.m_currencyAmount;

				auto slashingAccountBalance = accountCache.find(initialEntry.slashingAccount()).get().Balances.get(currencyId);

				ASSERT_EQ(slashingAccountBalance, currencySlashed);
			}
		}
	}

	TEST(TEST_CLASS, SlashingObserver_Success) {
		for (int i = 0; i < 1000; i++) {
			auto slashingPeriod = test::Random16();
			std::vector<LiquidityProviderInfo> initialInfos = { CreateInitialLInfo(slashingPeriod),
																CreateInitialLInfo(slashingPeriod) };

			CacheValues values(initialInfos, Height(initialInfos.back().entry.slashingPeriod()));

			// Assert
			RunTest(NotifyMode::Commit, values);
		}
	}

	TEST(TEST_CLASS, SlashingObserver_Rollback) {
		// Arrange:
		CacheValues values({}, Height(0));

		// Assert
		EXPECT_THROW(RunTest(NotifyMode::Rollback, values), catapult_runtime_error);
	}
}}