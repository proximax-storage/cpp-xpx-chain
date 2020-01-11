/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/config_holder/BlockchainConfigurationHolder.h"
#include "catapult/plugins/PluginUtils.h"
#include "src/config/ExchangeConfiguration.h"

namespace catapult { namespace cache {

	bool ExchangePluginEnabled(std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder, const Height& height) {
        const auto &blockchainConfig = pConfigHolder->Config(height);
        if (blockchainConfig.Network.Plugins.count(config::ExchangeConfiguration::Name)) {
            const auto &pluginConfig = blockchainConfig.Network.template GetPluginConfiguration<config::ExchangeConfiguration>();
            return pluginConfig.Enabled;
        }
        return false;
    }
}}