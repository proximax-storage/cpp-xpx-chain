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

	/// A validator implementation that applies to offer notification and validates that:
	/// - at least one offer is present
	/// - offer duration does not exceed maximum if transaction signer is not nemesis signer.
	/// - Mosaic amount is not zero.
	/// - Mosaic price is not zero.
	/// - Mosaic is allowed for exchange.
	DECLARE_STATEFUL_VALIDATOR(Offer, model::OfferNotification<1>)(const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder);

	/// A validator implementation that applies to matched offer notification and validates that:
	/// - matched offers exist.
	/// - matched offers are not expired.
	/// - matched offers are of valid type.
	/// - the offer and its matched offers are not announced by the same account.
	/// - matched offers have required mosaics.
	/// - matched offers have required mosaic amounts.
	/// - matched offers have valid mosaic prices.
	DECLARE_STATEFUL_VALIDATOR(MatchedOffer, model::MatchedOfferNotification<1>)();

	/// A validator implementation that applies to remove offer notification and validates that:
	/// - at least one offer is present.
	/// - offers exist.
	/// - transaction signer owns offers.
	DECLARE_STATEFUL_VALIDATOR(RemoveOffer, model::RemoveOfferNotification<1>)();

	/// A validator implementation that applies to plugin config notification and validates that:
	/// - plugin configuration is valid
	DECLARE_STATELESS_VALIDATOR(ExchangePluginConfig, model::PluginConfigNotification<1>)();
}}
