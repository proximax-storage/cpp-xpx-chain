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

#define TRY_LOAD_CHAIN_PROPERTY(NAME) utils::TryLoadIniProperty(bag, "", #NAME, config.NAME)
		config.MinGreedFeeInterest = 1u;
		TRY_LOAD_CHAIN_PROPERTY(MinGreedFeeInterest);
		config.MinGreedFeeInterestDenominator = 10u;
		TRY_LOAD_CHAIN_PROPERTY(MinGreedFeeInterestDenominator);
		config.ActivityScaleFactor = 1'000'000'000.0;
		TRY_LOAD_CHAIN_PROPERTY(ActivityScaleFactor);
		config.WeightScaleFactor = 1'000'000'000'000'000'000.0;
		TRY_LOAD_CHAIN_PROPERTY(WeightScaleFactor);
		config.EnableEqualWeights = false;
		TRY_LOAD_CHAIN_PROPERTY(EnableEqualWeights);
#undef TRY_LOAD_CHAIN_PROPERTY

		config.InitialActivityInt = static_cast<int64_t>(config.InitialActivity * config.ActivityScaleFactor);
		config.ActivityDeltaInt = static_cast<int64_t>(config.ActivityDelta * config.ActivityScaleFactor);
		config.ActivityCommitteeCosignedDeltaInt = static_cast<int64_t>(config.ActivityCommitteeCosignedDelta * config.ActivityScaleFactor);
		config.ActivityCommitteeNotCosignedDeltaInt = static_cast<int64_t>(config.ActivityCommitteeNotCosignedDelta * config.ActivityScaleFactor);

		return config;
	}
}}
