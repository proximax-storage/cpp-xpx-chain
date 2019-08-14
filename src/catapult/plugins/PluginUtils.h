/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once

#include "catapult/utils/Hashers.h"

namespace catapult { namespace plugins {

#define PLUGIN_NAME(PLUGIN) ("catapult.plugins." #PLUGIN)
#define PLUGIN_NAME_HASH(PLUGIN) HASH(PLUGIN_NAME(PLUGIN))

#define DEFINE_PLUGIN_CONFIG_VALIDATOR_WITH_FAILURE(PLUGIN_SUFFIX, CONFIG_NAME, FAILURE_NAME, VERSION) \
	using Notification = model::PluginConfigNotification<VERSION>;  \
	DEFINE_STATELESS_VALIDATOR(CONFIG_NAME##PluginConfig, [](const auto& notification) {  \
		if (notification.Name == PLUGIN_NAME(PLUGIN_SUFFIX)) {  \
			try {  \
				(void)config::CONFIG_NAME##Configuration::LoadFromBag(notification.Bag);  \
			} catch (...) {  \
				return FAILURE_NAME;  \
			}  \
		}  \
		return ValidationResult::Success;  \
	});

#define DEFINE_PLUGIN_CONFIG_VALIDATOR(PLUGIN_SUFFIX, CONFIG_NAME, VERSION) \
	DEFINE_PLUGIN_CONFIG_VALIDATOR_WITH_FAILURE(PLUGIN_SUFFIX, CONFIG_NAME, Failure_##CONFIG_NAME##_Plugin_Config_Malformed, VERSION)
}}
