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
		auto& entry = cache.find(notification.Owner).get();
		auto expiryHeight = entry.expiryHeight();

		auto pMosaic = notification.MosaicsPtr;
		for (uint8_t i = 0; i < notification.MosaicCount; ++i, ++pMosaic) {
			auto mosaicId = context.Resolvers.resolve(pMosaic->MosaicId);
			state::OfferBase& offer = (model::OfferType::Sell == pMosaic->OfferType) ?
				dynamic_cast<state::OfferBase&>(entry.sellOffers().at(mosaicId)) :
				dynamic_cast<state::OfferBase&>(entry.buyOffers().at(mosaicId));
			offer.ExpiryHeight = (NotifyMode::Commit == context.Mode) ? context.Height : offer.Deadline;
		}

		cache.updateExpiryHeight(notification.Owner, expiryHeight, entry.expiryHeight());
	});
}}
