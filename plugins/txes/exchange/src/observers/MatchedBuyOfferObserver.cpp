/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/cache/BuyOfferCache.h"
#include "src/cache/SellOfferCache.h"
#include <map>

namespace catapult { namespace observers {
	
	using Notification = model::MatchedBuyOfferNotification<1>;

	void MatchedBuyOfferObserver(const Notification& notification, const ObserverContext& context) {
		MatchedOfferObserver<Notification, cache::BuyOfferCache, cache::SellOfferCache>(notification, context);
	}
}}
