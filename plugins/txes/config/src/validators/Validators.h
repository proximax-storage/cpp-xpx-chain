/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "Results.h"
#include "src/model/NetworkConfigNotifications.h"
#include "catapult/plugins/PluginManager.h"
#include "catapult/validators/ValidatorTypes.h"

namespace catapult { namespace validators {
	/// A validator implementation that applies to network config signer notification and validates that:
	/// - signer is nemesis account
	DECLARE_STATEFUL_VALIDATOR(NetworkConfigSigner, model::NetworkConfigSignerNotification<1>)();

	/// A validator implementation that applies to network config notification and validates that:
	/// - blockchain configuration data size does not exceed the limit
	/// - no other config is declared at the same height
	/// - blockchain configuration data is valid
	/// - supported entity versions configuration data size does not exceed the limit
	/// - supported entity versions configuration data is valid
	DECLARE_STATEFUL_VALIDATOR(NetworkConfig, model::NetworkConfigNotification<1>)(const plugins::PluginManager& manager);

	/// A validator implementation that applies to plugin config notification and validates that:
	/// - plugin configuration is valid
	DECLARE_STATELESS_VALIDATOR(NetworkConfigPluginConfig, model::PluginConfigNotification<1>)();
}}
