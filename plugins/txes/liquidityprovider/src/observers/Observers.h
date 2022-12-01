/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/observers/ObserverTypes.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "src/cache/LiquidityProviderKeyCollector.h"
#include "src/model/InternalLiquidityProviderNotifications.h"
#include "src/catapult/model/LiquidityProviderNotifications.h"
#include "src/catapult/observers/LiquidityProviderExchangeObserver.h"

namespace catapult { namespace observers {

#define DEFINE_OBSERVER_WITH_LIQUIDITY_PROVIDER(NAME, NOTIFICATION_TYPE, HANDLER) \
	DECLARE_OBSERVER(NAME, NOTIFICATION_TYPE)(const std::unique_ptr<LiquidityProviderExchangeObserver>& liquidityProvider) { \
		return MAKE_OBSERVER(NAME, NOTIFICATION_TYPE, HANDLER); \
	}

	DECLARE_OBSERVER(Slashing, model::BlockNotification<1>)(const std::shared_ptr<cache::LiquidityProviderKeyCollector>& pKeyCollector);

	DECLARE_OBSERVER(CreateLiquidityProvider, model::CreateLiquidityProviderNotification<1>)();

	DECLARE_OBSERVER(DebitMosaic, model::DebitMosaicNotification<1>)(const std::unique_ptr<LiquidityProviderExchangeObserver>&);

	DECLARE_OBSERVER(CreditMosaic, model::CreditMosaicNotification<1>)(const std::unique_ptr<LiquidityProviderExchangeObserver>&);

	DECLARE_OBSERVER(ManualRateChange, model::ManualRateChangeNotification<1>)();

}}
