/**
*** Copyright (c) 2018-present,
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

#include "catapult/cache_core/BalanceView.h"
#include "tests/TestHarness.h"

namespace catapult { namespace cache {

#define TEST_CLASS BalanceViewTests

	auto SIGNER_KEY = Key({ { 1 } });

	constexpr auto Default_Cache_Options = AccountStateCacheTypes::Options{
			model::NetworkIdentifier::Mijin_Test,
			Amount(std::numeric_limits<Amount::ValueType>::max())
	};

	TEST(TEST_CLASS, BalanceViewGetBalanceCorrect) {
		// Arrange:
		AccountStateCache currentCache(CacheConfiguration(), Default_Cache_Options);
		{
			auto cacheDelta = currentCache.createDelta();
			auto& account = cacheDelta->addAccount(SIGNER_KEY, Height(0));
			account.Balances.credit(Xpx_Id, Amount(100));
			currentCache.commit();
		}

		AccountStateCache previousCache(CacheConfiguration(), Default_Cache_Options);

		// Act:
		auto currentCacheView = currentCache.createView();
		auto previousCacheView = previousCache.createView();

		BalanceView view(
				ReadOnlyAccountStateCache(*currentCacheView),
				ReadOnlyAccountStateCache(*previousCacheView),
				Height(0) /* effectiveBalanceHeight */
		);

		EXPECT_EQ(view.getBalance(SIGNER_KEY), Amount(100));
	}

	TEST(TEST_CLASS, BalanceViewGetBalanceCorrect_WhenDeltaIsChanged) {
		// Arrange:
		AccountStateCache currentCache(CacheConfiguration(), Default_Cache_Options);
		{
			auto cacheDelta = currentCache.createDelta();
			auto& account = cacheDelta->addAccount(SIGNER_KEY, Height(0));
			account.Balances.credit(Xpx_Id, Amount(100));
			currentCache.commit();
			cacheDelta->get(SIGNER_KEY).Balances.credit(Xpx_Id, Amount(1000));
		}

		AccountStateCache previousCache(CacheConfiguration(), Default_Cache_Options);

		// Act:
		auto currentCacheView = currentCache.createView();
		auto previousCacheView = previousCache.createView();

		BalanceView view(
			ReadOnlyAccountStateCache(*currentCacheView),
			ReadOnlyAccountStateCache(*previousCacheView),
			Height(0) /* effectiveBalanceHeight */
		);

		// Cache delta doesn't must effect current balance of account
		EXPECT_EQ(view.getBalance(SIGNER_KEY), Amount(100));
	}

	TEST(TEST_CLASS, BalanceViewGetBalanceCorrect_WhenPreviousCacheIsInitializedToo) {
		// Arrange:
		AccountStateCache currentCache(CacheConfiguration(), Default_Cache_Options);
		{
			auto cacheDelta = currentCache.createDelta();
			auto& account = cacheDelta->addAccount(SIGNER_KEY, Height(0));
			account.Balances.credit(Xpx_Id, Amount(100));
			currentCache.commit();
		}

		AccountStateCache previousCache(CacheConfiguration(), Default_Cache_Options);
		{
			auto cacheDelta = previousCache.createDelta();
			auto& account = cacheDelta->addAccount(SIGNER_KEY, Height(0));
			account.Balances.credit(Xpx_Id, Amount(200));
			previousCache.commit();
		}

		// Act:
		auto currentCacheView = currentCache.createView();
		auto previousCacheView = previousCache.createView();

		BalanceView view(
				ReadOnlyAccountStateCache(*currentCacheView),
				ReadOnlyAccountStateCache(*previousCacheView),
				Height(0) /* effectiveBalanceHeight */
		);

		// getBalance function must return value of current cache, not previous cache
		EXPECT_EQ(view.getBalance(SIGNER_KEY), Amount(100));
	}

	TEST(TEST_CLASS, BalanceViewGetEffectiveBalance) {
		// Arrange:
		AccountStateCache currentCache(CacheConfiguration(), Default_Cache_Options);
		{
			auto cacheDelta = currentCache.createDelta();
			auto& account = cacheDelta->addAccount(SIGNER_KEY, Height(0));
			account.Balances.credit(Xpx_Id, Amount(100));
			currentCache.commit();
		}

		AccountStateCache previousCache(CacheConfiguration(), Default_Cache_Options);
		{
			auto cacheDelta = previousCache.createDelta();
			auto& account = cacheDelta->addAccount(SIGNER_KEY, Height(0));
			account.Balances.credit(Xpx_Id, Amount(50));
			previousCache.commit();
		}

		// Act:
		auto currentCacheView = currentCache.createView();
		auto previousCacheView = previousCache.createView();

		BalanceView view(
				ReadOnlyAccountStateCache(*currentCacheView),
				ReadOnlyAccountStateCache(*previousCacheView),
				Height(1) /* effectiveBalanceHeight */
		);

		EXPECT_EQ(view.getEffectiveBalance(SIGNER_KEY, Height(1)), Amount(100));
		EXPECT_EQ(view.getEffectiveBalance(SIGNER_KEY, Height(2)), Amount(50));
	}

	TEST(TEST_CLASS, BalanceViewCanHarvest) {
		// Arrange:
		AccountStateCache currentCache(CacheConfiguration(), Default_Cache_Options);
		{
			auto cacheDelta = currentCache.createDelta();
			auto& account = cacheDelta->addAccount(SIGNER_KEY, Height(0));
			account.Balances.credit(Xpx_Id, Amount(100));
			currentCache.commit();
		}

		AccountStateCache previousCache(CacheConfiguration(), Default_Cache_Options);
		{
			auto cacheDelta = previousCache.createDelta();
			auto& account = cacheDelta->addAccount(SIGNER_KEY, Height(0));
			account.Balances.credit(Xpx_Id, Amount(50));
			previousCache.commit();
		}

		// Act:
		auto currentCacheView = currentCache.createView();
		auto previousCacheView = previousCache.createView();

		BalanceView view(
				ReadOnlyAccountStateCache(*currentCacheView),
				ReadOnlyAccountStateCache(*previousCacheView),
				Height(1) /* effectiveBalanceHeight */
		);

		EXPECT_TRUE(view.canHarvest(SIGNER_KEY, Height(1), Amount(100)));
		EXPECT_FALSE(view.canHarvest(SIGNER_KEY, Height(1), Amount(101)));
		EXPECT_TRUE(view.canHarvest(SIGNER_KEY, Height(2), Amount(50)));
		EXPECT_FALSE(view.canHarvest(SIGNER_KEY, Height(2), Amount(51)));
	}
}}
