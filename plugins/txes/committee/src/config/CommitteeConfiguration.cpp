/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "CommitteeConfiguration.h"
#include "catapult/utils/ConfigurationUtils.h"

namespace catapult { namespace config {

	CommitteeConfiguration CommitteeConfiguration::Uninitialized() {
		return CommitteeConfiguration();
	}

	CommitteeConfiguration CommitteeConfiguration::LoadFromBag(const utils::ConfigurationBag& bag) {
		CommitteeConfiguration config;

#define LOAD_PROPERTY(NAME) utils::LoadIniProperty(bag, "", #NAME, config.NAME)
		LOAD_PROPERTY(Enabled);
		LOAD_PROPERTY(MinGreed);
		LOAD_PROPERTY(InitialActivity);
		LOAD_PROPERTY(ActivityDelta);
		LOAD_PROPERTY(ActivityCommitteeCosignedDelta);
		LOAD_PROPERTY(ActivityCommitteeNotCosignedDelta);
#undef LOAD_PROPERTY

		utils::VerifyBagSizeLte(bag, PluginConfiguration::CommonPropertyNumber() + 6);
		return config;
	}
}}
