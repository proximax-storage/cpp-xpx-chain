/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "LicensingConfiguration.h"
#include "catapult/config/ConfigurationFileLoader.h"
#include "catapult/utils/ConfigurationUtils.h"

namespace catapult { namespace licensing {

#define LOAD_PROPERTY(SECTION, NAME) utils::LoadIniProperty(bag, SECTION, #NAME, config.NAME)

	LicensingConfiguration LicensingConfiguration::Uninitialized() {
		return LicensingConfiguration();
	}

	LicensingConfiguration LicensingConfiguration::LoadFromBag(const utils::ConfigurationBag& bag) {
		LicensingConfiguration config;

#define LOAD_LICENSING_PROPERTY(NAME) LOAD_PROPERTY("licensing", NAME)

		LOAD_LICENSING_PROPERTY(LicenseServerHost);
		LOAD_LICENSING_PROPERTY(LicenseServerPort);

#undef LOAD_LICENSING_PROPERTY

		utils::VerifyBagSizeLte(bag, 2);
		return config;
	}

#undef LOAD_PROPERTY

	LicensingConfiguration LicensingConfiguration::LoadFromPath(const boost::filesystem::path& resourcesPath) {
		return config::LoadIniConfiguration<LicensingConfiguration>(resourcesPath / "config-licensing.properties");
	}
}}
