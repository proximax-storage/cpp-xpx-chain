/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ModifyStateConfiguration.h"
#include "catapult/utils/ConfigurationUtils.h"

namespace catapult { namespace config {

	ModifyStateConfiguration ModifyStateConfiguration::Uninitialized() {
		return ModifyStateConfiguration();
	}

	ModifyStateConfiguration ModifyStateConfiguration::LoadFromBag(const utils::ConfigurationBag& bag) {
		ModifyStateConfiguration config;

#define LOAD_PROPERTY(NAME) utils::LoadIniProperty(bag, "", #NAME, config.NAME)
		LOAD_PROPERTY(Enabled);
#undef LOAD_PROPERTY

		return config;
	}
}}
