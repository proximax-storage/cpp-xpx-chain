/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/cache/SellOfferCache.h"

namespace catapult { namespace observers {

	using Notification = model::SellOfferNotification<1>;

	void SellOfferObserver(const Notification& notification, const ObserverContext& context) {
		OfferObserver<Notification, cache::SellOfferCache, false>(notification, context);
	}

	DEFINE_OBSERVER(SellOffer, Notification, SellOfferObserver)
}}
