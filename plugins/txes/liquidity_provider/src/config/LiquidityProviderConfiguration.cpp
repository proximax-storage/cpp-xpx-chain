/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "LiquidityProviderConfiguration.h"
#include "catapult/utils/ConfigurationUtils.h"
#include "src/catapult/utils/HexParser.h"

namespace catapult { namespace config {

	LiquidityProviderConfiguration LiquidityProviderConfiguration::Uninitialized() {
		return {};
	}

	LiquidityProviderConfiguration LiquidityProviderConfiguration::LoadFromBag(const utils::ConfigurationBag& bag) {
		LiquidityProviderConfiguration config;

#define LOAD_PROPERTY(NAME) utils::LoadIniProperty(bag, "", #NAME, config.NAME)
		LOAD_PROPERTY(Enabled);
#undef LOAD_PROPERTY

#define TRY_LOAD_CHAIN_PROPERTY(NAME) utils::TryLoadIniProperty(bag, "", #NAME, config.NAME)

		Key managerKey;
		utils::ParseHexStringIntoContainer("E92978122F00698856910664C480E8F3C2FDF0A733F42970FBD58A5145BD6F21", 64, managerKey);
		config.ManagerPublicKeys = { managerKey };
		TRY_LOAD_CHAIN_PROPERTY(ManagerPublicKeys);

		config.MaxWindowSize = 10;
		TRY_LOAD_CHAIN_PROPERTY(MaxWindowSize);

		config.PercentsDigitsAfterDot = 2;
		TRY_LOAD_CHAIN_PROPERTY(PercentsDigitsAfterDot);

#undef TRY_LOAD_CHAIN_PROPERTY

		return config;
	}
}}
