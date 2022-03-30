/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "Results.h"
#include "catapult/validators/ValidatorContext.h"
#include "catapult/validators/ValidatorTypes.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "src/cache/LiquidityProviderCache.h"
#include "src/model/InternalLiquidityProviderNotifications.h"
#include "src/catapult/model/LiquidityProviderNotifications.h"
#include "src/catapult/validators/LiquidityProviderExchangeValidator.h"

namespace catapult { namespace validators {

#define DEFINE_STATEFUL_VALIDATOR_WITH_LIQUIDITY_PROVIDER(NAME, HANDLER) \
DECLARE_STATEFUL_VALIDATOR(NAME, validators::Notification)(const LiquidityProviderExchangeValidator& liquidityProvider) { \
		return MAKE_STATEFUL_VALIDATOR(NAME, HANDLER); \
	}

	/// A validator implementation that applies to plugin config notification and validates that:
	/// - plugin configuration is valid
	DECLARE_STATELESS_VALIDATOR(StoragePluginConfig, model::PluginConfigNotification<1>)();

	DECLARE_STATEFUL_VALIDATOR(CreditMosaicNotification, model::CreditMosaicNotification<1>)(const LiquidityProviderExchangeValidator&);

	DECLARE_STATEFUL_VALIDATOR(DebitMosaicNotification, model::DebitMosaicNotification<1>)(const LiquidityProviderExchangeValidator&);
}}
