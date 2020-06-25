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

namespace catapult { namespace observers {

	class OfferExpiryUpdater {
	public:
		explicit OfferExpiryUpdater(cache::ExchangeCacheDelta& cache, state::ExchangeEntry& entry)
			: m_cache(cache)
			, m_entry(entry)
			, m_initialExpiryHeight(m_entry.minExpiryHeight())
			, m_initialPruneHeight(m_entry.minPruneHeight())
		{}

		~OfferExpiryUpdater();
		OfferExpiryUpdater(const OfferExpiryUpdater&) = delete;
		OfferExpiryUpdater& operator=(const OfferExpiryUpdater&) = delete;

	private:
		cache::ExchangeCacheDelta& m_cache;
		state::ExchangeEntry& m_entry;
		Height m_initialExpiryHeight;
		Height m_initialPruneHeight;
	};

	void CreditAccount(const Key& owner, const MosaicId& mosaicId, const Amount& amount, const ObserverContext &context);

	/// Observes changes triggered by offer notifications
	DECLARE_OBSERVER(OfferV1, model::OfferNotification<1>)();
	DECLARE_OBSERVER(OfferV2, model::OfferNotification<2>)();
	DECLARE_OBSERVER(OfferV3, model::OfferNotification<3>)();

	/// Observes changes triggered by exchange notifications
	DECLARE_OBSERVER(Exchange, model::ExchangeNotification<1>)();

	/// Observes changes triggered by remove offer notifications
	DECLARE_OBSERVER(RemoveOffer, model::RemoveOfferNotification<1>)(const MosaicId& currencyMosaicId);

	/// Observes changes triggered by block notifications
	DECLARE_OBSERVER(CleanupOffers, model::BlockNotification<1>)();
}}
