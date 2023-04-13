/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "DbrbConfiguration.h"
#include "catapult/config/ConfigurationFileLoader.h"
#include "catapult/crypto/KeyUtils.h"
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

		auto bootstrapProcesses = bag.getAllOrdered<bool>("bootstrap.processes");

		for (const auto& [key, enabled] : bootstrapProcesses) {
			if (enabled)
				config.BootstrapProcesses.emplace(crypto::ParseKey(key));
		}

		utils::VerifyBagSizeLte(bag, 1 + bootstrapProcesses.size());
		return config;
	}

#undef LOAD_PROPERTY

	DbrbConfiguration DbrbConfiguration::LoadFromPath(const boost::filesystem::path& resourcesPath) {
		return config::LoadIniConfiguration<DbrbConfiguration>(resourcesPath / "config-dbrb.properties");
	}
}}
