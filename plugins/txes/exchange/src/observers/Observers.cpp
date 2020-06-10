/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "catapult/cache_core/AccountStateCache.h"

namespace catapult { namespace observers {

	OfferExpiryUpdater::~OfferExpiryUpdater() {
		auto owner = m_entry.owner();
		std::set<Height> initialValues{m_initialExpiryHeight, m_initialPruneHeight};
		auto minExpiryHeight = m_entry.minExpiryHeight();
		auto minPruneHeight = m_entry.minPruneHeight();
		std::set<Height> newValues{minExpiryHeight, minPruneHeight};

		if (!newValues.count(m_initialPruneHeight))
			m_cache.removeExpiryHeight(owner, m_initialPruneHeight);

		if (!newValues.count(m_initialExpiryHeight))
			m_cache.removeExpiryHeight(owner, m_initialExpiryHeight);

		if (!initialValues.count(minPruneHeight))
			m_cache.addExpiryHeight(owner, minPruneHeight);

		if (!initialValues.count(minExpiryHeight))
			m_cache.addExpiryHeight(owner, minExpiryHeight);
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
