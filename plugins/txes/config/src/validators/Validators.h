/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "Results.h"
#include "src/model/CatapultConfigNotifications.h"
#include "catapult/plugins/PluginManager.h"
#include "catapult/validators/ValidatorTypes.h"

namespace catapult { namespace validators {
	/// A validator implementation that applies to catapult config signer notification and validates that:
	/// - signer is nemesis account
	DECLARE_STATEFUL_VALIDATOR(CatapultConfigSigner, model::CatapultConfigSignerNotification<1>)();

	/// A validator implementation that applies to catapult config notification and validates that:
	/// - blockchain configuration data size does not exceed the limit
	/// - no other config is declared at the same height
	/// - blockchain configuration data is valid
	/// - supported entity versions configuration data size does not exceed the limit
	/// - supported entity versions configuration data is valid
	DECLARE_STATEFUL_VALIDATOR(CatapultConfig, model::CatapultConfigNotification<1>)(const plugins::PluginManager& manager);

	/// A validator implementation that applies to plugin config notification and validates that:
	/// - plugin configuration is valid
	DECLARE_STATELESS_VALIDATOR(CatapultConfigPluginConfig, model::PluginConfigNotification<1>)();
}}
