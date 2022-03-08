/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "Results.h"
#include "src/model/SdaExchangeNotifications.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"
#include "catapult/validators/ValidatorContext.h"
#include "catapult/validators/ValidatorTypes.h"

namespace catapult { namespace validators {

	/// A validator implementation that applies to place and exchange offer notifications and validates that:
	/// - at least one offer to add is present
	/// - offer duration does not exceed maximum if transaction signer is not nemesis signer.
	/// - matched offers are not owned by the exchange signer.
	/// - account has offers.
	/// - Mosaic amount is not zero.
	/// - Mosaic price is not zero.
	/// - Mosaic is allowed for exchange.
    /// - matched offers exist.
	/// - matched offers have valid mosaic prices.
	/// - matched offers are not expired.
	/// - matched offers have required mosaic amounts.
	DECLARE_STATEFUL_VALIDATOR(PlaceSdaExchangeOfferV1, model::PlaceSdaOfferNotification<1>)();

	/// A validator implementation that applies to remove offer notification and validates that:
	/// - at least one offer to remove is present.
	/// - account has offers.
	/// - offers exist.
	/// - transaction signer owns offers.
	DECLARE_STATEFUL_VALIDATOR(RemoveSdaExchangeOfferV1, model::RemoveSdaOfferNotification<1>)();

	/// A validator implementation that applies to plugin config notification and validates that:
	/// - plugin configuration is valid
	DECLARE_STATELESS_VALIDATOR(SdaExchangePluginConfig, model::PluginConfigNotification<1>)();
}}
