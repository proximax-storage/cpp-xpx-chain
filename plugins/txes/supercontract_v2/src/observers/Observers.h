/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/observers/ObserverTypes.h"
#include "catapult/model/SupercontractNotifications.h"
#include "src/cache/SuperContractCache.h"
#include <queue>

namespace catapult::observers {

#define DEFINE_OBSERVER_WITH_LIQUIDITY_PROVIDER(NAME, NOTIFICATION_TYPE, HANDLER) \
	DECLARE_OBSERVER(NAME, NOTIFICATION_TYPE)(const LiquidityProviderExchangeObserver& liquidityProvider) { \
		return MAKE_OBSERVER(NAME, NOTIFICATION_TYPE, HANDLER); \
	}

	/// Observes changes triggered by prepare drive notifications.
	DECLARE_OBSERVER(DeployContract, model::DeploySupercontractNotification<1>)();
}
