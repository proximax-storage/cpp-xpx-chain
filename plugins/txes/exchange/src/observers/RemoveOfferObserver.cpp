/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/cache/OfferCache.h"

namespace catapult { namespace observers {
	
	using Notification = model::RemoveOfferNotification<1>;

	DEFINE_OBSERVER(RemoveOffer, Notification, [](const auto& notification, const ObserverContext& context) {
		auto& offerCache = context.Cache.sub<cache::OfferCache>();
		auto pHash = notification.OfferHashesPtr;
		for (uint8_t i = 0; i < notification.OfferCount; ++i, ++pHash) {
			auto& offerEntry = offerCache.find(*pHash).get();
			auto expiryHeight = NotifyMode::Commit == context.Mode ? context.Height : offerEntry.deadline();
			offerCache.updateExpiryHeight(*pHash, offerEntry.expiryHeight(), expiryHeight);
			offerEntry.setExpiryHeight(expiryHeight);
		}
	});
}}
