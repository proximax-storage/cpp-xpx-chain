/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "Results.h"
#include "src/model/ExchangeNotifications.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"
#include "catapult/validators/ValidatorContext.h"
#include "catapult/validators/ValidatorTypes.h"

namespace catapult { namespace validators {

	/// A validator implementation that applies to offer notifications and validates that:
	/// - at least one offer to add is present
	/// - offer duration does not exceed maximum if transaction signer is not nemesis signer.
	/// - Mosaic amount is not zero.
	/// - Mosaic price is not zero.
	/// - Mosaic is allowed for exchange.
	DECLARE_STATEFUL_VALIDATOR(OfferV1, model::OfferNotification<1>)();
	DECLARE_STATEFUL_VALIDATOR(OfferV2, model::OfferNotification<2>)();
	DECLARE_STATEFUL_VALIDATOR(OfferV3, model::OfferNotification<3>)();
	DECLARE_STATEFUL_VALIDATOR(OfferV4, model::OfferNotification<4>)();

	/// A validator implementation that applies to exchange notification and validates that:
	/// - at least one offer to exchange is present.
	/// - matched offers are not owned by the exchange signer.
	/// - account has offers.
	/// - matched offers exist.
	/// - matched offers have valid mosaic prices.
	/// - matched offers are not expired.
	/// - matched offers have required mosaic amounts.
	DECLARE_STATEFUL_VALIDATOR(ExchangeV1, model::ExchangeNotification<1>)();
	DECLARE_STATEFUL_VALIDATOR(ExchangeV2, model::ExchangeNotification<2>)();

	/// A validator implementation that applies to remove offer notification and validates that:
	/// - at least one offer to remove is present.
	/// - account has offers.
	/// - offers exist.
	/// - transaction signer owns offers.
	DECLARE_STATEFUL_VALIDATOR(RemoveOfferV1, model::RemoveOfferNotification<1>)();
	DECLARE_STATEFUL_VALIDATOR(RemoveOfferV2, model::RemoveOfferNotification<2>)();

	/// A validator implementation that applies to plugin config notification and validates that:
	/// - plugin configuration is valid
	DECLARE_STATELESS_VALIDATOR(ExchangePluginConfig, model::PluginConfigNotification<1>)();
}}
