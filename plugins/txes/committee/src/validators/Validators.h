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

namespace catapult { namespace chain { class WeightedVotingCommitteeManager; }}

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
}}
