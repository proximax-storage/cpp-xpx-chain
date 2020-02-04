/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "OperationConfiguration.h"
#include "catapult/utils/ConfigurationUtils.h"

namespace catapult { namespace config {

	OperationConfiguration OperationConfiguration::Uninitialized() {
		return OperationConfiguration();
	}

	OperationConfiguration OperationConfiguration::LoadFromBag(const utils::ConfigurationBag& bag) {
		OperationConfiguration config;

#define LOAD_PROPERTY(NAME) utils::LoadIniProperty(bag, "", #NAME, config.NAME)
		LOAD_PROPERTY(Enabled);
		LOAD_PROPERTY(MaxOperationDuration);
#undef LOAD_PROPERTY

		utils::VerifyBagSizeLte(bag, PluginConfiguration::CommonPropertyNumber() + 2);
		return config;
	}
}}
