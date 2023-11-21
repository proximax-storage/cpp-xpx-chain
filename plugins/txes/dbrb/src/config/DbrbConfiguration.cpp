/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "DbrbConfiguration.h"
#include "catapult/utils/ConfigurationUtils.h"

namespace catapult { namespace config {

	DbrbConfiguration DbrbConfiguration::Uninitialized() {
		return DbrbConfiguration();
	}

	DbrbConfiguration DbrbConfiguration::LoadFromBag(const utils::ConfigurationBag& bag) {
		DbrbConfiguration config;

#define LOAD_PROPERTY(NAME) utils::LoadIniProperty(bag, "", #NAME, config.NAME)
		LOAD_PROPERTY(Enabled);
#undef LOAD_PROPERTY

		utils::VerifyBagSizeLte(bag, PluginConfiguration::CommonPropertyNumber() + 1);

		return config;
	}
}}
