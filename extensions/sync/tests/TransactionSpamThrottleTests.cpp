/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#include "catapult/cache/CatapultCache.h"
#include "catapult/cache/MemoryUtCache.h"
#include "catapult/cache/ReadOnlyCatapultCache.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "catapult/model/BlockChainConfiguration.h"
#include "sync/src/TransactionSpamThrottle.h"
#include "tests/test/cache/CacheTestUtils.h"
#include "tests/test/core/TransactionInfoTestUtils.h"
#include "tests/test/core/TransactionTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace sync {

#define TEST_CLASS TransactionSpamThrottleTests

	namespace {
		using TransactionSource = chain::UtUpdater::TransactionSource;

		cache::CatapultCache CreateCatapultCache() {
			auto blockChainConfiguration = model::BlockChainConfiguration::Uninitialized();
			return test::CreateEmptyCatapultCache(blockChainConfiguration);
		}

		const state::AccountState& AddAccount(cache::AccountStateCacheDelta& delta, Amount balance) {
			auto& accountState = delta.addAccount(test::GenerateRandomData<Key_Size>(), Height(1));
			accountState.Balances.credit(Xpx_Id, balance);
			return accountState;
		}

		std::vector<const state::AccountState*> SeedAccountStateCache(
				cache::AccountStateCacheDelta& delta,
				size_t count,
				Amount balance) {
			std::vector<const state::AccountState*> accountStates;
			for (auto i = 0u; i < count; ++i) {
				accountStates.push_back(&AddAccount(delta, balance));
			}

			return accountStates;
		}

		model::TransactionInfo CreateTransactionInfo(const Key& signer, Amount fee = Amount()) {
			auto pTransaction = test::GenerateRandomTransaction(signer);
			pTransaction->Fee = fee;
			auto transactionInfo = model::TransactionInfo(std::move(pTransaction));
			test::FillWithRandomData(transactionInfo.EntityHash);
			return transactionInfo;
		}

		class TestContext {
		public:
			explicit TestContext(cache::CatapultCache&& catapultCache, Height height)
					: m_catapultCache(std::move(catapultCache))
					, m_catapultCacheView(m_catapultCache.createView())
					, m_readOnlyCatapultCache(m_catapultCacheView.toReadOnly())
					, m_height(height)
					, m_transactionsCache(cache::MemoryCacheOptions(1024, 100'000))
					, m_transactionsCacheModifier(m_transactionsCache.modifier())
			{}

		public:
			cache::UtCacheModifierProxy& transactionsCacheModifier() {
				return m_transactionsCacheModifier;
			}

			chain::UtUpdater::ThrottleContext throttleContext(TransactionSource source = TransactionSource::New) const {
				return { source, m_height, m_readOnlyCatapultCache, m_transactionsCacheModifier };
			}

			void addTransactions(const std::vector<const state::AccountState*>& accountStates) {
				for (const auto* pAccountState : accountStates)
					m_transactionsCacheModifier.add(CreateTransactionInfo(pAccountState->PublicKey));
			}

		private:
			cache::CatapultCache m_catapultCache;
			cache::CatapultCacheView m_catapultCacheView;
			cache::ReadOnlyCatapultCache m_readOnlyCatapultCache;
			Height m_height;
			cache::MemoryUtCacheProxy m_transactionsCache;
			cache::UtCacheModifierProxy m_transactionsCacheModifier;
		};

		// region ThrottleTestSettings + factory functions

		enum class TransactionBondPolicy { Unbonded, Bonded };

		struct ThrottleTestSettings {
			SpamThrottleConfiguration ThrottleConfig{ Amount(10'000'000), Amount(1'000'000), 1'200, 120 };
			uint32_t CacheSize = 120;
			Amount DefaultBalance = Amount(1'000);
			Amount SignerBalance = Amount();
			Amount Fee = Amount();
			TransactionSource Source = TransactionSource::New;
			TransactionBondPolicy BondPolicy = TransactionBondPolicy::Unbonded;
		};

		ThrottleTestSettings CreateSettingsWithCacheSize(uint32_t cacheSize) {
			ThrottleTestSettings settings;
			settings.CacheSize = cacheSize;
			return settings;
		}

		ThrottleTestSettings CreateSettingsWithSignerBalance(Amount signerBalance) {
			ThrottleTestSettings settings;
			settings.SignerBalance = signerBalance;
			return settings;
		}

		ThrottleTestSettings CreateSettingsWithFee(Amount fee) {
			ThrottleTestSettings settings;
			settings.Fee = fee;
			return settings;
		}

		ThrottleTestSettings CreateSettingsWithFee(Amount fee, Amount maxBoostFee) {
			ThrottleTestSettings settings;
			settings.Fee = fee;
			settings.ThrottleConfig.MaxBoostFee = maxBoostFee;
			return settings;
		}

		ThrottleTestSettings CreateSettings(uint32_t cacheSize, Amount signerBalance, Amount fee) {
			ThrottleTestSettings settings;
			settings.CacheSize = cacheSize;
			settings.SignerBalance = signerBalance;
			settings.Fee = fee;
			return settings;
		}

		ThrottleTestSettings CreateSettings(uint32_t cacheMaxSize, uint32_t cacheSize, TransactionSource source) {
			ThrottleTestSettings settings;
			settings.ThrottleConfig.MaxCacheSize = cacheMaxSize;
			settings.ThrottleConfig.MaxBlockSize = 10;
			settings.CacheSize = cacheSize;
			settings.DefaultBalance = Amount();
			settings.Source = source;
			return settings;
		}

		ThrottleTestSettings CreateSettings(uint32_t cacheMaxSize, uint32_t cacheSize, TransactionBondPolicy bondPolicy) {
			auto settings = CreateSettings(cacheMaxSize, cacheSize, TransactionSource::New);
			settings.BondPolicy = bondPolicy;
			return settings;
		}

		// endregion

		void AssertThrottling(const ThrottleTestSettings& settings, bool expectedResult) {
			// Arrange: prepare account state cache
			auto catapultCache = CreateCatapultCache();
			std::vector<const state::AccountState*> accountStates;
			const state::AccountState* pAccountState;
			{
				auto delta = catapultCache.createDelta();
				auto& accountStateCacheDelta = delta.sub<cache::AccountStateCache>();
				accountStates = SeedAccountStateCache(accountStateCacheDelta, settings.CacheSize, settings.DefaultBalance);
				pAccountState = &AddAccount(accountStateCacheDelta, settings.SignerBalance);
				catapultCache.commit(Height(1));
			}

			TestContext context(std::move(catapultCache), Height(1));
			context.addTransactions(accountStates);

			// Sanity:
			EXPECT_EQ(settings.CacheSize, context.transactionsCacheModifier().size());

			std::vector<const model::Transaction*> transactions;
			auto isBonded = [&transactions, bondPolicy = settings.BondPolicy](const auto& transaction) {
				transactions.push_back(&transaction);
				return TransactionBondPolicy::Bonded == bondPolicy;
			};
			auto throttleContext = context.throttleContext(settings.Source);
			auto transactionInfo = CreateTransactionInfo(pAccountState->PublicKey, settings.Fee);
			auto filter = CreateTransactionSpamThrottle(settings.ThrottleConfig, isBonded);

			// Act:
			auto result = filter(transactionInfo, throttleContext);

			// Assert:
			EXPECT_EQ(expectedResult, result) << "for balance " << settings.SignerBalance;
			auto expectedTransactions =
					settings.CacheSize >= settings.ThrottleConfig.MaxBlockSize && settings.CacheSize < settings.ThrottleConfig.MaxCacheSize
					? std::vector<const model::Transaction*>{ transactionInfo.pEntity.get() }
					: std::vector<const model::Transaction*>();
			EXPECT_EQ(expectedTransactions, transactions) << "for balance " << settings.SignerBalance;
		}
	}

	// region comparison of cache size to block size

	TEST(TEST_CLASS, TransactionIsNotFilteredWhenCacheSizeIsSmallerThanBlockSize) {
		// Act: max block size is 120
		AssertThrottling(CreateSettingsWithCacheSize(0), false);
		AssertThrottling(CreateSettingsWithCacheSize(1), false);
		AssertThrottling(CreateSettingsWithCacheSize(10), false);
		AssertThrottling(CreateSettingsWithCacheSize(119), false);
	}

	TEST(TEST_CLASS, TransactionIsFilteredWhenCacheSizeIsEqualToBlockSizeAndTransactionDoesNotMeetRequirements) {
		// Act: max block size is 120
		AssertThrottling(CreateSettingsWithCacheSize(120), true);
	}

	TEST(TEST_CLASS, TransactionIsFilteredWhenCacheSizeIsGreaterThanBlockSizeAndTransactionDoesNotMeetRequirements) {
		// Act: max block size is 120
		AssertThrottling(CreateSettingsWithCacheSize(121), true);
		AssertThrottling(CreateSettingsWithCacheSize(200), true);
		AssertThrottling(CreateSettingsWithCacheSize(500), true);
	}

	// endregion

	// region balance

	TEST(TEST_CLASS, BalanceIncreasesUsefulBalance) {
		// Act: signer has no balance
		AssertThrottling(CreateSettingsWithSignerBalance(Amount()), true);

		// Act: signer has 0.1% balance of total balance, transaction is accepted
		AssertThrottling(CreateSettingsWithSignerBalance(Amount(1'000)), false);
	}

	// endregion

	// region fee

	TEST(TEST_CLASS, TransactionFeeIncreasesUsefulBalance) {
		// Act: no fee, transaction is rejected
		AssertThrottling(CreateSettingsWithFee(Amount()), true);

		// Act: high fee boosts the useful balance to 1% of total balance:
		//      - total balance = 1'000'000, fee = 10 xpx, so attemptedBalanceBoost = 10'000
		AssertThrottling(CreateSettingsWithFee(Amount(10'000'000)), false);
	}

	TEST(TEST_CLASS, MaxBoostFeeAffectsHowTransactionFeeIncreasesUsefulBalance) {
		// Act: the max boost fee is 10 xpx, a low fee is not enough to get the transactions accepted
		//      - low fee boosts the useful balance by only 0.001% of total balance:
		//      - total balance = 1'000'000, fee = 0.01 xpx, max boost fee = 10 xpx, so attemptedBalanceBoost = 10
		AssertThrottling(CreateSettingsWithFee(Amount(10'000), Amount(10'000'000)), true);

		// Act: the max boost fee is 0.01 xpx, even a low fee is enough to get the transactions accepted
		//      - even a low fee boosts the useful balance to 1% of total balance:
		//      - total balance = 1'000'000, fee = 0.01 xpx, max boost fee = 0.01 xpx, so attemptedBalanceBoost = 10'000
		AssertThrottling(CreateSettingsWithFee(Amount(10'000), Amount(10'000)), false);
	}

	// endregion

	// region throttling - single account / multiple accounts

	namespace {
		enum class AccountPolicy { Single, Multiple };

		void AssertThrottling(const ThrottleTestSettings& settings, AccountPolicy accountPolicy, uint32_t expectedCacheSize) {
			// Arrange:
			auto config = settings.ThrottleConfig;
			auto catapultCache = CreateCatapultCache();
			std::vector<const state::AccountState*> accountStates;
			{
				auto delta = catapultCache.createDelta();
				auto& accountStateCacheDelta = delta.sub<cache::AccountStateCache>();
				accountStates = SeedAccountStateCache(accountStateCacheDelta, config.MaxCacheSize, settings.SignerBalance);
				catapultCache.commit(Height(1));
			}

			TestContext context(std::move(catapultCache), Height(1));

			// Sanity:
			EXPECT_EQ(0u, context.transactionsCacheModifier().size());

			auto throttleContext = context.throttleContext();

			bool isFiltered = false;
			auto index = 0u;
			while (!isFiltered && config.MaxCacheSize > context.transactionsCacheModifier().size()) {
				auto transactionInfo = CreateTransactionInfo(accountStates[index]->PublicKey, settings.Fee);
				auto filter = CreateTransactionSpamThrottle(config, [](const auto&) { return false; });

				// Act:
				isFiltered = filter(transactionInfo, throttleContext);

				// Assert:
				auto cacheSize = context.transactionsCacheModifier().size();
				if (!isFiltered && !context.transactionsCacheModifier().add(transactionInfo))
					CATAPULT_THROW_RUNTIME_ERROR_1("transaction could not be added to cache at size", cacheSize);

				if (AccountPolicy::Multiple == accountPolicy)
					++index;
			}

			// Assert:
			EXPECT_EQ(expectedCacheSize, context.transactionsCacheModifier().size()) << "for balance " << settings.SignerBalance;
		}
	}

	TEST(TEST_CLASS, ThrottlingBehavesAsExpected_SingleAccount) {
		// Arrange:
		// - single account fills the cache
		// - rounded solutions for equation: balance * e^(-3 * y / 1200) * 100 * (1200 - y) = y
		// - for a relative balance <= 0.001, only max transactions per block (120) are allowed when cache is initially empty
		std::vector<uint64_t> rawBalances{ 1'000'000, 100'000, 10'000, 1000, 100, 10 };
		std::vector<uint32_t> expectedCacheSizes{ 1054, 736, 352, 120, 120, 120 };
		for (auto i = 0u; i < rawBalances.size(); ++i) {
			auto settings = CreateSettings(0, Amount(rawBalances[i]), Amount());
			AssertThrottling(settings, AccountPolicy::Single, expectedCacheSizes[i]);
		}
	}

	TEST(TEST_CLASS, ThrottlingBehavesAsExpected_MultipleAccounts) {
		// Arrange:
		// - different accounts fill the cache
		// - rounded solutions for equation: balance * e^(-3 * y / 1200) * 100 * (1200 - y) = 1
		std::vector<uint64_t> rawBalances{ 1'000'000, 100'000, 10'000, 1000, 100, 10 };
		std::vector<uint32_t> expectedCacheSizes{ 1200, 1199, 1181, 1059, 669, 120 };
		for (auto i = 0u; i < rawBalances.size(); ++i) {
			auto settings = CreateSettings(0, Amount(rawBalances[i]), Amount());
			AssertThrottling(settings, AccountPolicy::Multiple, expectedCacheSizes[i]);
		}
	}

	// endregion

	// region throttling - different fill levels

	TEST(TEST_CLASS, ThrottlingBehavesAsExpected_FullCache) {
		// Act + Assert: max balance to try to force acceptance
		AssertThrottling(CreateSettings(1199, Amount(1'000'000), Amount()), false); // almost full
		AssertThrottling(CreateSettings(1200, Amount(1'000'000), Amount()), true); // full
	}

	TEST(TEST_CLASS, ThrottlingBehavesAsExpected_HighFillLevel) {
		// Act + Assert:
		AssertThrottling(CreateSettings(1000, Amount(), Amount()), true); // no balance and fee
		AssertThrottling(CreateSettings(1000, Amount(), Amount(100'000)), true); // no balance, medium fee
		AssertThrottling(CreateSettings(1000, Amount(), Amount(1000'000)), false); // no balance, high fee

		AssertThrottling(CreateSettings(1000, Amount(100), Amount()), true); // medium balance and no fee
		AssertThrottling(CreateSettings(1000, Amount(100), Amount(100'000)), true); // medium balance and fee

		AssertThrottling(CreateSettings(1000, Amount(10'000), Amount()), false); // high balance and no fee
		AssertThrottling(CreateSettings(1000, Amount(10'000), Amount(1'000'000)), false); // high balance and fee
	}

	TEST(TEST_CLASS, ThrottlingBehavesAsExpected_MediumFillLevel) {
		// Act + Assert:
		AssertThrottling(CreateSettings(250, Amount(), Amount()), true); // no balance and fee
		AssertThrottling(CreateSettings(250, Amount(), Amount(100'000)), false); // no balance, medium fee
		AssertThrottling(CreateSettings(250, Amount(), Amount(1000'000)), false); // no balance, high fee

		AssertThrottling(CreateSettings(250, Amount(100), Amount()), false); // medium balance and no fee
		AssertThrottling(CreateSettings(250, Amount(100), Amount(100'000)), false); // medium balance and fee

		AssertThrottling(CreateSettings(250, Amount(10'000), Amount()), false); // high balance and no fee
		AssertThrottling(CreateSettings(250, Amount(10'000), Amount(1'000'000)), false); // high balance and fee
	}

	TEST(TEST_CLASS, ThrottlingBehavesAsExpected_LowFillLevel) {
		// Act + Assert:
		AssertThrottling(CreateSettings(100, Amount(), Amount()), false); // no balance and fee
		AssertThrottling(CreateSettings(100, Amount(), Amount(100'000)), false); // no balance, medium fee
		AssertThrottling(CreateSettings(100, Amount(), Amount(1000'000)), false); // no balance, high fee

		AssertThrottling(CreateSettings(100, Amount(100), Amount()), false); // medium balance and no fee
		AssertThrottling(CreateSettings(100, Amount(100), Amount(100'000)), false); // medium balance and fee

		AssertThrottling(CreateSettings(100, Amount(10'000), Amount()), false); // high balance and no fee
		AssertThrottling(CreateSettings(100, Amount(10'000), Amount(1'000'000)), false); // high balance and fee
	}

	// endregion

	// region throttling - transaction source

	TEST(TEST_CLASS, ThrottlingBehavesAsExpected_TransactionSourceNew) {
		// Act + Assert: block size is 10, no balance and fee
		AssertThrottling(CreateSettings(100, 9, TransactionSource::New), false); // cache size < block max size
		AssertThrottling(CreateSettings(100, 10, TransactionSource::New), true); // cache size == block max size
		AssertThrottling(CreateSettings(100, 50, TransactionSource::New), true); // cache size > block max size
	}

	TEST(TEST_CLASS, ThrottlingBehavesAsExpected_TransactionSourceExisting) {
		// Act + Assert: block size is 10, no balance and fee
		AssertThrottling(CreateSettings(100, 9, TransactionSource::Existing), false); // cache size < block max size
		AssertThrottling(CreateSettings(100, 10, TransactionSource::Existing), true); // cache size == block max size
		AssertThrottling(CreateSettings(100, 50, TransactionSource::Existing), true); // cache size > block max size
	}

	TEST(TEST_CLASS, ThrottlingBehavesAsExpected_TransactionSourceReverted) {
		// Act + Assert: block size is 10, no balance and fee
		AssertThrottling(CreateSettings(100, 50, TransactionSource::Reverted), false); // cache is half full
		AssertThrottling(CreateSettings(100, 99, TransactionSource::Reverted), false); // cache is almost full
		AssertThrottling(CreateSettings(100, 100, TransactionSource::Reverted), true); // cache is full
	}

	// endregion

	// region throttling - bonded transactions

	TEST(TEST_CLASS, ThrottlingBehavesAsExpected_BondedTransactions) {
		// Act + Assert: block size is 10, no balance and fee
		AssertThrottling(CreateSettings(100, 50, TransactionBondPolicy::Bonded), false); // cache is half full
		AssertThrottling(CreateSettings(100, 99, TransactionBondPolicy::Bonded), false); // cache is almost full
		AssertThrottling(CreateSettings(100, 100, TransactionBondPolicy::Bonded), true); // cache is full
	}

	TEST(TEST_CLASS, ThrottlingBehavesAsExpected_UnbondedTransactions) {
		// Act + Assert: block size is 10, no balance and fee
		AssertThrottling(CreateSettings(100, 9, TransactionBondPolicy::Unbonded), false); // cache size < block max size
		AssertThrottling(CreateSettings(100, 10, TransactionBondPolicy::Unbonded), true); // cache size == block max size
		AssertThrottling(CreateSettings(100, 50, TransactionBondPolicy::Unbonded), true); // cache size > block max size
	}

	// endregion
}}
