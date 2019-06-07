/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "CatapultConfigConfiguration.h"
#include "catapult/utils/ConfigurationBag.h"
#include "catapult/utils/ConfigurationUtils.h"

namespace catapult { namespace config {

	CatapultConfigConfiguration CatapultConfigConfiguration::Uninitialized() {
		return CatapultConfigConfiguration();
	}

	CatapultConfigConfiguration CatapultConfigConfiguration::LoadFromBag(const utils::ConfigurationBag& bag) {
		CatapultConfigConfiguration config;

#define LOAD_PROPERTY(NAME) utils::LoadIniProperty(bag, "", #NAME, config.NAME)
		LOAD_PROPERTY(CatapultConfigTransactionSupportedVersions);
		LOAD_PROPERTY(MaxBlockChainConfigSize);
#undef LOAD_PROPERTY

		utils::VerifyBagSizeLte(bag, PluginConfiguration::CommonPropertyNumber() + 2);
		return config;
	}
}}
