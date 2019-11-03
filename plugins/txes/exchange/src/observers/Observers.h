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
		explicit OfferExpiryUpdater(cache::ExchangeCacheDelta& cache, state::ExchangeEntry& entry)
			: m_cache(cache)
			, m_entry(entry)
			, m_initialExpiryHeight(m_entry.minExpiryHeight())
			, m_initialPruneHeight(m_entry.minPruneHeight())
		{}

		~OfferExpiryUpdater() {
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

			if (m_entry.empty())
				m_cache.remove(owner);
		}

		OfferExpiryUpdater(const OfferExpiryUpdater&) = delete;
		OfferExpiryUpdater& operator=(const OfferExpiryUpdater&) = delete;

	private:
		cache::ExchangeCacheDelta& m_cache;
		state::ExchangeEntry& m_entry;
		Height m_initialExpiryHeight;
		Height m_initialPruneHeight;
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
