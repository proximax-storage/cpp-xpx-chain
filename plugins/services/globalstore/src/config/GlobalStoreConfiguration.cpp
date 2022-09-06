/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/


#include "GlobalStoreConfiguration.h"
#include "catapult/utils/ConfigurationBag.h"
#include "catapult/utils/ConfigurationUtils.h"

namespace catapult { namespace config {

	GlobalStoreConfiguration GlobalStoreConfiguration::Uninitialized() {
		return GlobalStoreConfiguration();
	}

	GlobalStoreConfiguration GlobalStoreConfiguration::LoadFromBag(const utils::ConfigurationBag& bag) {
		GlobalStoreConfiguration config;

#define LOAD_PROPERTY(NAME) utils::LoadIniProperty(bag, "", #NAME, config.NAME)
		LOAD_PROPERTY(Enabled);
#undef LOAD_PROPERTY

		utils::VerifyBagSizeLte(bag, 1);
		return config;
	}
}}
