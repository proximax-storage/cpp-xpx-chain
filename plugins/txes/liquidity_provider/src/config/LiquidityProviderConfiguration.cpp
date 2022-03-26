/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "LiquidityProviderConfiguration.h"
#include "catapult/utils/ConfigurationUtils.h"

namespace catapult { namespace config {

	LiquidityProviderConfiguration LiquidityProviderConfiguration::Uninitialized() {
		return StorageConfiguration();
	}

	LiquidityProviderConfiguration LiquidityProviderConfiguration::LoadFromBag(const utils::ConfigurationBag& bag) {
		StorageConfiguration config;

#define LOAD_PROPERTY(NAME) utils::LoadIniProperty(bag, "", #NAME, config.NAME)
		LOAD_PROPERTY(Enabled);
#undef LOAD_PROPERTY

#define TRY_LOAD_CHAIN_PROPERTY(NAME) utils::TryLoadIniProperty(bag, "", #NAME, config.NAME)

#undef TRY_LOAD_CHAIN_PROPERTY

		return config;
	}
}}
