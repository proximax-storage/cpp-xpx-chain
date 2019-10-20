/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/cache/OfferDeadlineCache.h"

namespace catapult { namespace observers {
	
	using Notification = model::RemoveOfferNotification<1>;

	DEFINE_OBSERVER(RemoveOffer, Notification, [](const auto& notification, const ObserverContext& context) {
		auto& offerDeadlineCache = context.Cache.sub<cache::OfferDeadlineCache>();
		if (!offerDeadlineCache.contains(Height(0)))
			offerDeadlineCache.insert(state::OfferDeadlineEntry(Height(0)));
		auto& offerDeadlineEntry = offerDeadlineCache.find(Height(0)).get();

		auto pHash = notification.BuyOfferHashesPtr;
		for (uint8_t i = 0; i < notification.BuyOfferCount; ++i, ++pHash) {
			offerDeadlineEntry.buyOfferHeights().emplace(context.Height, *pHash);
		}

		pHash = notification.SellOfferHashesPtr;
		for (uint8_t i = 0; i < notification.SellOfferCount; ++i, ++pHash) {
			offerDeadlineEntry.sellOfferHeights().emplace(context.Height, *pHash);
		}
	});
}}
