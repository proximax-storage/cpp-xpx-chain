/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "SuperContractV2Configuration.h"
#include "catapult/utils/ConfigurationUtils.h"

namespace catapult { namespace config {

	SuperContractV2Configuration SuperContractV2Configuration::Uninitialized() {
		return SuperContractV2Configuration();
	}

	SuperContractV2Configuration SuperContractV2Configuration::LoadFromBag(const utils::ConfigurationBag& bag) {
		SuperContractV2Configuration config;

#define LOAD_PROPERTY(NAME) utils::LoadIniProperty(bag, "", #NAME, config.NAME)
		LOAD_PROPERTY(Enabled);
#undef LOAD_PROPERTY

#define TRY_LOAD_CHAIN_PROPERTY(NAME) utils::TryLoadIniProperty(bag, "", #NAME, config.NAME)

		config.MaxServicePaymentsSize = 512U;
		TRY_LOAD_CHAIN_PROPERTY(MaxServicePaymentsSize);

		config.MaxRowSize = 4096U;
		TRY_LOAD_CHAIN_PROPERTY(MaxRowSize);

		config.MaxExecutionPayment = 1000000U;
		TRY_LOAD_CHAIN_PROPERTY(MaxRowSize);

		config.MaxAutoExecutions = 100000U;
		TRY_LOAD_CHAIN_PROPERTY(MaxAutoExecutions);

		config.AutomaticExecutionsDeadline = Height(5760U);
		TRY_LOAD_CHAIN_PROPERTY(AutomaticExecutionsDeadline);

#undef TRY_LOAD_CHAIN_PROPERTY
		return config;
	}
}}
