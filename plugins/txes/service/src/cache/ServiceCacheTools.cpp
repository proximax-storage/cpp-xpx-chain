/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/config_holder/BlockchainConfigurationHolder.h"
#include "catapult/functions.h"
#include "src/config/ServiceConfiguration.h"

namespace catapult { namespace cache {

	namespace {
		using PropertyGetter = std::function<bool (const config::ServiceConfiguration&)>;

		bool GetProperty(std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder, const Height& height, const PropertyGetter& propertyGetter) {
			const auto& blockchainConfig = pConfigHolder->Config(height);
			if (blockchainConfig.Network.Plugins.count(config::ServiceConfiguration::Name)) {
				const auto& pluginConfig = blockchainConfig.Network.template GetPluginConfiguration<config::ServiceConfiguration>();
				return propertyGetter(pluginConfig);
			}

			return false;
		}
	}

	bool ServicePluginEnabled(std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder, const Height& height) {
		return GetProperty(pConfigHolder, height, [](const auto& config) { return config.Enabled; });
	}

	bool DownloadCacheEnabled(std::shared_ptr<config::BlockchainConfigurationHolder> pConfigHolder, const Height& height) {
		return GetProperty(pConfigHolder, height, [](const auto& config) { return config.DownloadCacheEnabled; });
	}
}}