/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/model/CommitteeNotifications.h"
#include "catapult/observers/ObserverTypes.h"

namespace catapult { namespace chain { class WeightedVotingCommitteeManager; }}

namespace catapult { namespace observers {

	/// Observes changes triggered by add harvester notifications
	DECLARE_OBSERVER(AddHarvester, model::AddHarvesterNotification<1>)();

	/// Observes changes triggered by remove harvester notifications
	DECLARE_OBSERVER(RemoveHarvester, model::RemoveHarvesterNotification<1>)();

	/// Observes changes triggered by block cosignatures notifications
	DECLARE_OBSERVER(UpdateHarvesters, model::BlockCosignaturesNotification<1>)(
		const std::shared_ptr<chain::WeightedVotingCommitteeManager>& pCommitteeManager);
}}
