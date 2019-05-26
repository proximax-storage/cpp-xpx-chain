/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/config/MultisigConfiguration.h"

namespace catapult { namespace validators {

	using Notification = model::PluginConfigNotification<1>;

	DEFINE_STATELESS_VALIDATOR(PluginConfig, [](const auto& notification) {
		if (notification.Name == "catapult.plugins.multisig") {
			try {
				(void)config::MultisigConfiguration::LoadFromBag(notification.Bag);
			} catch (...) {
				return Failure_Multisig_Plugin_Config_Malformed;
			}
		}

		return ValidationResult::Success;
	});
}}
