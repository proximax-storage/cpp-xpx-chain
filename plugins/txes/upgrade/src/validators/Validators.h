/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "Results.h"
#include "src/model/BlockchainUpgradeNotifications.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"
#include "catapult/validators/ValidatorContext.h"
#include "catapult/validators/ValidatorTypes.h"

namespace catapult { namespace validators {
	/// A validator implementation that applies to blockchain upgrade signer notification and validates that:
	/// - signer is nemesis account
	DECLARE_STATEFUL_VALIDATOR(BlockchainUpgradeSigner, model::BlockchainUpgradeSignerNotification<1>)();

	/// A validator implementation that applies to blockchain account upgrades from V1 to V2 and validates that:
	/// - signer is a valid V1 account
	/// - V2 accounts are allowed
	/// - Account does not already exist
	DECLARE_STATEFUL_VALIDATOR(AccountV2Upgrade, model::AccountV2UpgradeNotification<1>)();

	/// A validator implementation that applies to blockchain upgrade notification and validates that:
	/// - upgrade period is valid (greater or equal the minimum value set in config)
	/// - no other upgrade is declared at the same height
	DECLARE_STATEFUL_VALIDATOR(BlockchainUpgrade, model::BlockchainUpgradeVersionNotification<1>)();

	/// A validator implementation that applies to all block notifications and validates that:
	/// - the blockchain version is valid
	DECLARE_STATEFUL_VALIDATOR(BlockchainVersion, model::BlockNotification<1>)();

	/// A validator implementation that applies to plugin config notification and validates that:
	/// - plugin configuration is valid
	DECLARE_STATELESS_VALIDATOR(BlockchainUpgradePluginConfig, model::PluginConfigNotification<1>)();
}}
