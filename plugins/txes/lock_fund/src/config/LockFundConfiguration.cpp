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
#undef LOAD_PROPERTY

#define TRY_LOAD_CHAIN_PROPERTY(NAME) utils::TryLoadIniProperty(bag, "", #NAME, config.NAME)

			config.MaxMosaicsSize = 256;
			TRY_LOAD_CHAIN_PROPERTY(MinRequestUnlockCooldown);
			config.MinRequestUnlockCooldown = BlockDuration(161280);
			TRY_LOAD_CHAIN_PROPERTY(MinRequestUnlockCooldown);

#undef TRY_LOAD_CHAIN_PROPERTY
		return config;
	}
}}
