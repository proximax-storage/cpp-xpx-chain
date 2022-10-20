/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ContractConfiguration.h"
#include "catapult/utils/ConfigurationUtils.h"

namespace catapult { namespace config {

	ContractConfiguration ContractConfiguration::Uninitialized() {
		return ContractConfiguration();
	}

	ContractConfiguration ContractConfiguration::LoadFromBag(const utils::ConfigurationBag& bag) {
		ContractConfiguration config;

#define LOAD_PROPERTY(NAME) utils::LoadIniProperty(bag, "", #NAME, config.NAME)
		LOAD_PROPERTY(Enabled);
#undef LOAD_PROPERTY

		return config;
	}
}}
