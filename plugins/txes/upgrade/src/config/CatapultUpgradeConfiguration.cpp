/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "CatapultUpgradeConfiguration.h"
#include "catapult/utils/ConfigurationBag.h"
#include "catapult/utils/ConfigurationUtils.h"

namespace catapult { namespace config {

	CatapultUpgradeConfiguration CatapultUpgradeConfiguration::Uninitialized() {
		return CatapultUpgradeConfiguration();
	}

	CatapultUpgradeConfiguration CatapultUpgradeConfiguration::LoadFromBag(const utils::ConfigurationBag& bag) {
		CatapultUpgradeConfiguration config;

#define LOAD_PROPERTY(NAME) utils::LoadIniProperty(bag, "", #NAME, config.NAME)
		LOAD_PROPERTY(MinUpgradePeriod);
#undef LOAD_PROPERTY

		utils::VerifyBagSizeLte(bag, 1);
		return config;
	}
}}
