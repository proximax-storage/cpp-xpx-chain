/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "catapult/cache_core/AccountStateCache.h"

namespace catapult { namespace observers {

	OfferExpiryUpdater::~OfferExpiryUpdater() {
		const auto& owner = m_entry.owner();
		auto minExpiryHeight = m_entry.minExpiryHeight();
		auto minPruneHeight = m_entry.minPruneHeight();

		m_cache.addExpiryHeight(owner, minPruneHeight);
		m_cache.addExpiryHeight(owner, minExpiryHeight);

		// We don't need touch cache. It is old code, so we support removing for old versions of transaction.
		if (m_entry.version() < 3 && m_entry.empty())
			m_cache.remove(owner);
	}

	void CreditAccount(const Key& owner, const MosaicId& mosaicId, const Amount& amount, const ObserverContext &context) {
		if (Amount(0) != amount) {
			auto &cache = context.Cache.sub<cache::AccountStateCache>();
			auto iter = cache.find(owner);
			auto &recipientState = iter.get();
			if (NotifyMode::Commit == context.Mode)
				recipientState.Balances.credit(mosaicId, amount, context.Height);
			else
				recipientState.Balances.debit(mosaicId, amount, context.Height);
		}
	}
}}
