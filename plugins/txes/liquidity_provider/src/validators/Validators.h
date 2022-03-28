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
#include "src/model/LiquidityProviderNotifications.h"

namespace catapult { namespace validators {

	/// A validator implementation that applies to plugin config notification and validates that:
	/// - plugin configuration is valid
	DECLARE_STATELESS_VALIDATOR(StoragePluginConfig, model::PluginConfigNotification<1>)();
}}
