/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "LockFundConfiguration.h"
#include "catapult/utils/ConfigurationBag.h"
#include "catapult/utils/ConfigurationUtils.h"

namespace catapult { namespace config {

		LockFundConfiguration LockFundConfiguration::Uninitialized() {
		return LockFundConfiguration();
	}

		LockFundConfiguration LockFundConfiguration::LoadFromBag(const utils::ConfigurationBag& bag) {
			LockFundConfiguration config;
		utils::LoadIniProperty(bag, "", "MaxMosaicsSize", config.MaxMosaicsSize);
		utils::LoadIniProperty(bag, "", "MinRequestUnlockCooldown", config.MinRequestUnlockCooldown);
		utils::VerifyBagSizeLte(bag, PluginConfiguration::CommonPropertyNumber() + 2);
		return config;
	}
}}
