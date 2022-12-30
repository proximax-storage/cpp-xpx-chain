/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ExecutorConfiguration.h"
#include "catapult/config/ConfigurationFileLoader.h"
#include "catapult/utils/ConfigurationUtils.h"

namespace catapult { namespace contract {

#define LOAD_PROPERTY(SECTION, NAME) utils::LoadIniProperty(bag, SECTION, #NAME, config.NAME)

	ExecutorConfiguration ExecutorConfiguration::Uninitialized() {
		return ExecutorConfiguration();
	}

	ExecutorConfiguration ExecutorConfiguration::LoadFromBag(const utils::ConfigurationBag& bag) {
		ExecutorConfiguration config;

#define LOAD_DB_PROPERTY(NAME) LOAD_PROPERTY("exxecutor", NAME)

		LOAD_DB_PROPERTY(Key);

#undef LOAD_DB_PROPERTY

		utils::VerifyBagSizeLte(bag, 1);
		return config;
	}

#undef LOAD_PROPERTY

	ExecutorConfiguration ExecutorConfiguration::LoadFromPath(const boost::filesystem::path& resourcesPath) {
		return config::LoadIniConfiguration<ExecutorConfiguration>(resourcesPath / "config-storage.properties");
	}
}}
