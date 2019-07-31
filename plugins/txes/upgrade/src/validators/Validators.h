/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "Results.h"
#include "src/model/CatapultUpgradeNotifications.h"
#include "catapult/config_holder/LocalNodeConfigurationHolder.h"
#include "catapult/validators/ValidatorContext.h"
#include "catapult/validators/ValidatorTypes.h"

namespace catapult { namespace validators {
	/// A validator implementation that applies to catapult upgrade signer notification and validates that:
	/// - signer is nemesis account
	DECLARE_STATEFUL_VALIDATOR(CatapultUpgradeSigner, model::CatapultUpgradeSignerNotification<1>)();

	/// A validator implementation that applies to catapult upgrade notification and validates that:
	/// - upgrade period is valid (greater or equal the minimum value set in config)
	/// - no other upgrade is declared at the same height
	DECLARE_STATEFUL_VALIDATOR(CatapultUpgrade, model::CatapultUpgradeVersionNotification<1>)(const std::shared_ptr<config::LocalNodeConfigurationHolder>& pConfigHolder);

	/// A validator implementation that applies to all block notifications and validates that:
	/// - the catapult version is valid
	DECLARE_STATEFUL_VALIDATOR(CatapultVersion, model::BlockNotification<1>)();

	/// A validator implementation that applies to plugin config notification and validates that:
	/// - plugin configuration is valid
	DECLARE_STATELESS_VALIDATOR(PluginConfig, model::PluginConfigNotification<1>)();
}}
