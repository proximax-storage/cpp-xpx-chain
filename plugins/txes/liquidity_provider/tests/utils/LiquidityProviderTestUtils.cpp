/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "LiquidityProviderTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult::test {
	void AddAccountState(
			cache::AccountStateCacheDelta& accountStateCache,
			const Key& publicKey,
			const Height& height,
			const std::vector<model::Mosaic>& mosaics) {
		accountStateCache.addAccount(publicKey, height);
		auto accountStateIter = accountStateCache.find(publicKey);
		auto& accountState = accountStateIter.get();
		for (auto& mosaic : mosaics)
			accountState.Balances.credit(mosaic.MosaicId, mosaic.Amount);
	}
}