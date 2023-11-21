/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "SuperContractConfiguration.h"
#include "catapult/utils/ConfigurationUtils.h"

namespace catapult { namespace config {

	SuperContractConfiguration SuperContractConfiguration::Uninitialized() {
		return SuperContractConfiguration();
	}

	SuperContractConfiguration SuperContractConfiguration::LoadFromBag(const utils::ConfigurationBag& bag) {
		SuperContractConfiguration config;

#define LOAD_PROPERTY(NAME) utils::LoadIniProperty(bag, "", #NAME, config.NAME)
		LOAD_PROPERTY(Enabled);
#undef LOAD_PROPERTY

#define TRY_LOAD_CHAIN_PROPERTY(NAME) utils::TryLoadIniProperty(bag, "", #NAME, config.NAME)

        config.MaxSuperContractsOnDrive = 10;
        TRY_LOAD_CHAIN_PROPERTY(MaxSuperContractsOnDrive);

#undef TRY_LOAD_CHAIN_PROPERTY

		return config;
	}
}}
