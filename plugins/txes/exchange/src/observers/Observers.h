/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/model/NotificationSubscriber.h"
#include "catapult/observers/ObserverTypes.h"
#include "src/cache/ExchangeCache.h"
#include "src/model/ExchangeNotifications.h"
#include "src/state/ExchangeEntry.h"

namespace catapult { namespace observers {

	class OfferExpiryUpdater {
	public:
		explicit OfferExpiryUpdater(cache::ExchangeCacheDelta& cache, state::ExchangeEntry& entry, bool removeIfEmpty)
			: m_cache(cache)
			, m_entry(entry)
			, m_initialExpiryHeight(m_entry.minExpiryHeight())
			, m_removeIfEmpty(removeIfEmpty)
		{}

		~OfferExpiryUpdater() {
			auto owner = m_entry.owner();
			m_cache.updateExpiryHeight(owner, m_initialExpiryHeight, m_entry.minExpiryHeight());
			if (m_removeIfEmpty && m_entry.empty())
				m_cache.remove(owner);
		}

		OfferExpiryUpdater(const OfferExpiryUpdater&) = delete;
		OfferExpiryUpdater& operator=(const OfferExpiryUpdater&) = delete;

	private:
		cache::ExchangeCacheDelta& m_cache;
		state::ExchangeEntry& m_entry;
		Height m_initialExpiryHeight;
		bool m_removeIfEmpty;
	};

	/// Observes changes triggered by offer notifications
	DECLARE_OBSERVER(Offer, model::OfferNotification<1>)();

	/// Observes changes triggered by exchange notifications
	DECLARE_OBSERVER(Exchange, model::ExchangeNotification<1>)();

	/// Observes changes triggered by remove offer notifications
	DECLARE_OBSERVER(RemoveOffer, model::RemoveOfferNotification<1>)();

	/// Observes changes triggered by block notifications
	DECLARE_OBSERVER(CleanupOffers, model::BlockNotification<1>)(const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder);
}}
