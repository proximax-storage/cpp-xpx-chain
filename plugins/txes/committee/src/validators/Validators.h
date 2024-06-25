/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "Results.h"
#include "src/model/CommitteeNotifications.h"
#include "catapult/validators/ValidatorContext.h"
#include "catapult/validators/ValidatorTypes.h"

namespace catapult { namespace chain { class WeightedVotingCommitteeManagerV3; }}

namespace catapult { namespace validators {
	/// A validator implementation that applies to add harvester notification and validates that:
	/// - harvester is not registered yet
	/// - harvester has sufficient effective balance
	DECLARE_STATEFUL_VALIDATOR(AddHarvester, model::AddHarvesterNotification<1>)();

	/// A validator implementation that applies to remove harvester notification and validates that:
	/// - harvester is registered
	DECLARE_STATEFUL_VALIDATOR(RemoveHarvester, model::RemoveHarvesterNotification<1>)();

	/// A validator implementation that applies to plugin config notification and validates that:
	/// - plugin configuration is valid
	DECLARE_STATELESS_VALIDATOR(CommitteePluginConfig, model::PluginConfigNotification<1>)();

	/// A validator implementation that applies to block committee notification V4 and validates that:
	/// - selected committee round is the same as in block
	/// - block signer is block proposer
	DECLARE_STATEFUL_VALIDATOR(Committee, model::BlockCommitteeNotification<4>)(const std::shared_ptr<chain::WeightedVotingCommitteeManagerV3>& pCommitteeManager);
}}
