/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ServiceConfiguration.h"
#include "catapult/utils/ConfigurationBag.h"
#include "catapult/utils/ConfigurationUtils.h"

namespace catapult { namespace config {

	ServiceConfiguration ServiceConfiguration::Uninitialized() {
		return ServiceConfiguration();
	}

	ServiceConfiguration ServiceConfiguration::LoadFromBag(const utils::ConfigurationBag& bag) {
		ServiceConfiguration config;

#define LOAD_PROPERTY(NAME) utils::LoadIniProperty(bag, "", #NAME, config.NAME)
		LOAD_PROPERTY(Enabled);
#undef LOAD_PROPERTY

#define TRY_LOAD_CHAIN_PROPERTY(NAME) utils::TryLoadIniProperty(bag, "", #NAME, config.NAME)

		config.MaxFilesOnDrive = 32768;
		TRY_LOAD_CHAIN_PROPERTY(MaxFilesOnDrive);
		config.VerificationFee = Amount(10);
		TRY_LOAD_CHAIN_PROPERTY(VerificationFee);
		config.VerificationDuration = BlockDuration(240);
		TRY_LOAD_CHAIN_PROPERTY(VerificationDuration);
		config.DownloadDuration = BlockDuration(40320);
		TRY_LOAD_CHAIN_PROPERTY(DownloadDuration);

#undef TRY_LOAD_CHAIN_PROPERTY

		return config;
	}
}}
