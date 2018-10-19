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

	TEST(TEST_CLASS, BalanceViewCanHarvest) {
		// Arrange:
		AccountStateCache cache(CacheConfiguration(), Default_Cache_Options);
		{
			auto cacheDelta = cache.createDelta();
			auto& account = cacheDelta->addAccount(SIGNER_KEY, Height(0));
			account.Balances.credit(Xpx_Id, Amount(100), Height(0));
			cache.commit();
		}

		// Act:
		auto cacheView = cache.createView();

		BalanceView view(
				ReadOnlyAccountStateCache(*cacheView),
				Height(1) /* effectiveBalanceHeight */
		);

		EXPECT_TRUE(view.canHarvest(SIGNER_KEY, Height(1), Amount(100)));
	}
}}
