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
			auto deadline = Height(-1);
			if (notification.Duration.unwrap() < BlockDuration(-1).unwrap() - context.Height.unwrap())
				deadline = Height(context.Height.unwrap() + notification.Duration.unwrap());
			state::OfferEntry entry(notification.TransactionHash, notification.Signer, notification.OfferType, deadline);
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
