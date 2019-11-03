/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/cache/ExchangeCache.h"

namespace catapult { namespace observers {
	
	using Notification = model::RemoveOfferNotification<1>;

	DEFINE_OBSERVER(RemoveOffer, Notification, [](const auto& notification, const ObserverContext& context) {
		auto& cache = context.Cache.sub<cache::ExchangeCache>();
		auto iter = cache.find(notification.Owner);
		auto& entry = iter.get();
		OfferExpiryUpdater offerExpiryUpdater(cache, entry);

		auto pOffer = notification.OffersPtr;
		for (uint8_t i = 0; i < notification.OfferCount; ++i, ++pOffer) {
			auto mosaicId = context.Resolvers.resolve(pOffer->MosaicId);
			if (NotifyMode::Commit == context.Mode)
				entry.expireOffer(pOffer->OfferType, mosaicId, context.Height);
			else
				entry.unexpireOffer(pOffer->OfferType, mosaicId, context.Height);
		}
	});
}}
