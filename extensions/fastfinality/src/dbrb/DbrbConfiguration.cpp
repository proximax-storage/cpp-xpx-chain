/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "DbrbConfiguration.h"
#include "catapult/config/ConfigurationFileLoader.h"
#include "catapult/utils/ConfigurationUtils.h"

namespace catapult { namespace dbrb {

#define LOAD_PROPERTY(SECTION, NAME) utils::LoadIniProperty(bag, SECTION, #NAME, config.NAME)

	DbrbConfiguration DbrbConfiguration::Uninitialized() {
		return DbrbConfiguration();
	}

	DbrbConfiguration DbrbConfiguration::LoadFromBag(const utils::ConfigurationBag& bag) {
		DbrbConfiguration config;

#define LOAD_DB_PROPERTY(NAME) LOAD_PROPERTY("dbrb", NAME)

		LOAD_DB_PROPERTY(TransactionTimeout);

#undef LOAD_DB_PROPERTY

#define TRY_LOAD_CHAIN_PROPERTY(NAME) utils::TryLoadIniProperty(bag, "dbrb", #NAME, config.NAME)

		config.IsDbrbProcess = true;
		TRY_LOAD_CHAIN_PROPERTY(IsDbrbProcess);
		config.ResendMessagesInterval = utils::TimeSpan::FromMilliseconds(500);
		TRY_LOAD_CHAIN_PROPERTY(ResendMessagesInterval);

#undef TRY_LOAD_CHAIN_PROPERTY

		return config;
	}

#undef LOAD_PROPERTY

	DbrbConfiguration DbrbConfiguration::LoadFromPath(const boost::filesystem::path& resourcesPath) {
		return config::LoadIniConfiguration<DbrbConfiguration>(resourcesPath / "config-dbrb.properties");
	}
}}
