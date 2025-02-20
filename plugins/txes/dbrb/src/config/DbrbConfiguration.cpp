/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "DbrbConfiguration.h"
#include "catapult/utils/ConfigurationUtils.h"

namespace catapult { namespace config {

	DbrbConfiguration DbrbConfiguration::Uninitialized() {
		return {};
	}

	DbrbConfiguration DbrbConfiguration::LoadFromBag(const utils::ConfigurationBag& bag) {
		DbrbConfiguration config;

#define LOAD_PROPERTY(NAME) utils::LoadIniProperty(bag, "", #NAME, config.NAME)
		LOAD_PROPERTY(Enabled);
#undef LOAD_PROPERTY

#define TRY_LOAD_CHAIN_PROPERTY(NAME) utils::TryLoadIniProperty(bag, "", #NAME, config.NAME)
		config.DbrbProcessLifetimeAfterExpiration = utils::TimeSpan();
		TRY_LOAD_CHAIN_PROPERTY(DbrbProcessLifetimeAfterExpiration);
		config.EnableDbrbProcessBanning = false;
		TRY_LOAD_CHAIN_PROPERTY(EnableDbrbProcessBanning);
#undef TRY_LOAD_CHAIN_PROPERTY

		return config;
	}
}}
