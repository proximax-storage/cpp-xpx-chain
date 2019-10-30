/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include <plugins/txes/exchange/src/cache/ExchangeCache.h>
#include "catapult/model/NotificationSubscriber.h"
#include "catapult/observers/ObserverTypes.h"
#include "src/model/ExchangeNotifications.h"
#include "src/state/ExchangeEntry.h"

namespace catapult { namespace observers {

	/// Observes changes triggered by offer notifications
	DECLARE_OBSERVER(Offer, model::OfferNotification<1>)();

	/// Observes changes triggered by exchange notifications
	DECLARE_OBSERVER(Exchange, model::ExchangeNotification<1>)();

	/// Observes changes triggered by remove offer notifications
	DECLARE_OBSERVER(RemoveOffer, model::RemoveOfferNotification<1>)();

	/// Observes changes triggered by block notifications
	DECLARE_OBSERVER(CleanupOffers, model::BlockNotification<1>)(const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder);
}}
