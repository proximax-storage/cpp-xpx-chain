/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"

namespace catapult { namespace observers {

	DEFINE_OBSERVER(Offer, model::OfferNotification<1>, [](const auto& notification, const ObserverContext& context) {
		auto& cache = context.Cache.sub<cache::OfferCache>();
		if (NotifyMode::Commit == context.Mode) {
			auto maxDeadline = std::numeric_limits<Height::ValueType>::max();
			auto duration = notification.Duration.unwrap();
			auto currentHeight = context.Height.unwrap();
			auto deadline = (duration < maxDeadline - currentHeight) ? currentHeight + duration : maxDeadline;
			state::OfferEntry entry(notification.TransactionHash, notification.Signer, notification.OfferType, Height(deadline));
			auto pOffer = notification.OffersPtr;
			for (uint8_t i = 0; i < notification.OfferCount; ++i, ++pOffer) {
				entry.initialOffers().emplace(pOffer->Mosaic.MosaicId, *pOffer);
				entry.offers().emplace(pOffer->Mosaic.MosaicId, *pOffer);
			}
			cache.insert(entry);
		} else {
			cache.remove(notification.TransactionHash);
		}
	});
}}
