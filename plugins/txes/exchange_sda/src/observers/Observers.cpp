/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "catapult/cache_core/AccountStateCache.h"

namespace catapult { namespace observers {

	SdaOfferExpiryUpdater::~SdaOfferExpiryUpdater() {
		const auto& owner = m_entry.owner();
		auto minExpiryHeight = m_entry.minExpiryHeight();
		auto minPruneHeight = m_entry.minPruneHeight();

		m_cache.addExpiryHeight(owner, minPruneHeight);
		m_cache.addExpiryHeight(owner, minExpiryHeight);
	}

	void CreditAccount(const Key& owner, const MosaicId& mosaicId, const Amount& amount, const ObserverContext &context) {
		if (Amount(0) != amount) {
			auto &cache = context.Cache.sub<cache::AccountStateCache>();
			auto iter = cache.find(owner);
			auto &recipientState = iter.get();
			recipientState.Balances.credit(mosaicId, amount, context.Height);
		}
	}
}}
