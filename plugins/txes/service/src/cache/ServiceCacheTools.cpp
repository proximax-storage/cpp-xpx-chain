/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/config_holder/BlockchainConfigurationHolder.h"
#include "catapult/plugins/PluginUtils.h"
#include "src/config/ServiceConfiguration.h"

namespace catapult { namespace cache {

	bool ServicePluginEnabled(std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder, const Height& height) {
		const auto& networkConfig = pConfigHolder->Config(height).Network;
		if (networkConfig.Plugins.count(PLUGIN_NAME(service))) {
            const auto& pluginConfig = networkConfig.GetPluginConfiguration<config::ServiceConfiguration>(PLUGIN_NAME_HASH(service));
            return pluginConfig.Enabled;
		}

		return false;
	}
}}