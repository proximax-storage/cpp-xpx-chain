/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "BlockchainUpgradeConfiguration.h"
#include "catapult/utils/ConfigurationBag.h"
#include "catapult/utils/ConfigurationUtils.h"

namespace catapult { namespace config {

	BlockchainUpgradeConfiguration BlockchainUpgradeConfiguration::Uninitialized() {
		return BlockchainUpgradeConfiguration();
	}

	BlockchainUpgradeConfiguration BlockchainUpgradeConfiguration::LoadFromBag(const utils::ConfigurationBag& bag) {
		BlockchainUpgradeConfiguration config;

#define LOAD_PROPERTY(NAME) utils::LoadIniProperty(bag, "", #NAME, config.NAME)
		LOAD_PROPERTY(MinUpgradePeriod);
#undef LOAD_PROPERTY

		utils::VerifyBagSizeLte(bag, PluginConfiguration::CommonPropertyNumber() + 1);
		return config;
	}
}}
