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

#include "BalanceTransferTestUtils.h"
#include "CacheTestUtils.h"
#include "catapult/cache/CatapultCache.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "tests/TestHarness.h"

namespace catapult { namespace test {

	namespace {
		template<typename TKey>
		void SetCacheBalancesT(cache::CatapultCacheDelta& cache, const TKey& key, const BalanceTransfers& transfers) {
			auto& accountStateCache = cache.sub<cache::AccountStateCache>();
			auto& accountState = accountStateCache.addAccount(key, Height(123));
			for (const auto& transfer : transfers)
				accountState.Balances.credit(transfer.MosaicId, transfer.Amount, Height(123));
		}
	}

	void SetCacheBalances(cache::CatapultCacheDelta& cache, const Key& publicKey, const BalanceTransfers& transfers) {
		SetCacheBalancesT(cache, publicKey, transfers);
	}

	void SetCacheBalances(cache::CatapultCacheDelta& cache, const Address& address, const BalanceTransfers& transfers) {
		SetCacheBalancesT(cache, address, transfers);
	}

	void SetCacheBalances(cache::CatapultCache& cache, const Key& publicKey, const BalanceTransfers& transfers) {
		auto delta = cache.createDelta();
		SetCacheBalances(delta, publicKey, transfers);
		cache.commit(Height());
	}

	cache::CatapultCache CreateCache(const Key& publicKey, const BalanceTransfers& transfers) {
		auto cache = CreateEmptyCatapultCache();
		SetCacheBalances(cache, publicKey, transfers);
		return cache;
	}
}}
