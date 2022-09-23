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
#define LOAD_PROPERTY(NAME) utils::LoadIniProperty(bag, "", #NAME, config.NAME)
			LOAD_PROPERTY(Enabled);
			LOAD_PROPERTY(MaxMosaicsSize);
			LOAD_PROPERTY(MaxUnlockRequests);
			LOAD_PROPERTY(MinRequestUnlockCooldown);
			LOAD_PROPERTY(DockStakeRewardInterval);
#undef LOAD_PROPERTY
			utils::VerifyBagSizeLte(bag, PluginConfiguration::CommonPropertyNumber() + 5);
		return config;
	}
}}
