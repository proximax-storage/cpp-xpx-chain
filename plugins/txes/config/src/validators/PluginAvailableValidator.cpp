/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "Results.h"
#include "catapult/validators/ValidatorContext.h"
#include "catapult/utils/ConfigurationUtils.h"
#include "catapult/extensions/ExtensionManager.h"

namespace catapult { namespace validators {
	using Notification = model::BlockNotification<1>;

	auto Validate(const plugins::PluginManager& pluginManager){
		return [&pluginManager](const auto& notification, const ValidatorContext& context) {
			// Check if a new configuration has become active.
			if(context.Config.ActivationHeight == context.Height && context.Config.PreviousConfiguration != nullptr) {
				//This is the block at which this configuration becomes active. Let's verify if the addon plugin list has changed.
				//Retrieve boot configuration
				auto bootConfig = pluginManager.configHolder()->Config(Height(0));

				// Make sure that the boot config plugins match the config that's about to be used
				auto matches = 0;
				for(auto& plugin : context.Config.Network.Plugins) {
					for(auto& activePlugin : bootConfig.Network.Plugins) {
						if(activePlugin.first == plugin.first) {
							matches++;
							break;
						}
					}
				}
				if(matches == bootConfig.Network.Plugins.size() && matches == context.Config.Network.Plugins.size()) return ValidationResult::Success;
				return Failure_NetworkConfig_Required_Plugins_Not_Matching;
			}
			return ValidationResult::Success;
		};
	}

	DECLARE_STATEFUL_VALIDATOR(PluginAvailable, Notification)(const plugins::PluginManager& pluginManager) {
		return MAKE_STATEFUL_VALIDATOR(PluginAvailable, Validate(pluginManager));
	}
}}
