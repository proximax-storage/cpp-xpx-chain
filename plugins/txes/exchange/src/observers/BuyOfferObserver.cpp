/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/cache/BuyOfferCache.h"

namespace catapult { namespace observers {

	using Notification = model::BuyOfferNotification<1>;

	void BuyOfferObserver(const Notification& notification, const ObserverContext& context) {
		OfferObserver<Notification, cache::BuyOfferCache, true>(notification, context);
	}

	DEFINE_OBSERVER(BuyOffer, Notification, BuyOfferObserver)
}}
