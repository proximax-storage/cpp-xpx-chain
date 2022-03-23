/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/model/NotificationSubscriber.h"
#include "catapult/observers/ObserverTypes.h"
#include "src/cache/SdaOfferGroupCache.h"
#include "src/cache/SdaExchangeCache.h"
#include "src/model/SdaExchangeNotifications.h"

namespace catapult { namespace observers {

	class SdaOfferExpiryUpdater {
	public:
		explicit SdaOfferExpiryUpdater(cache::SdaExchangeCacheDelta& cache, state::SdaExchangeEntry& entry)
			: m_cache(cache)
			, m_entry(entry)
		{}

		~SdaOfferExpiryUpdater();
		SdaOfferExpiryUpdater(const SdaOfferExpiryUpdater&) = delete;
		SdaOfferExpiryUpdater& operator=(const SdaOfferExpiryUpdater&) = delete;

	private:
		cache::SdaExchangeCacheDelta& m_cache;
		state::SdaExchangeEntry& m_entry;
	};

	void CreditAccount(const Key& owner, const MosaicId& mosaicId, const Amount& amount, const ObserverContext &context);

	int denominator(int mosaicGive, int mosaicGet);
    std::string reducedFraction(Amount mosaicGive, Amount mosaicGet);
    Hash256 calculateGroupHash(MosaicId mosaicGiveId, MosaicId mosaicGetId, std::string reduced);

	/// Observes changes triggered by place and exchange offer notifications
	DECLARE_OBSERVER(PlaceSdaExchangeOfferV1, model::PlaceSdaOfferNotification<1>)();

	/// Observes changes triggered by remove offer notifications
	DECLARE_OBSERVER(RemoveSdaExchangeOfferV1, model::RemoveSdaOfferNotification<1>)();

	/// Observes changes triggered by block notifications
	DECLARE_OBSERVER(CleanupSdaOffers, model::BlockNotification<1>)();
}}
