/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/model/CommitteeNotifications.h"
#include "catapult/observers/ObserverTypes.h"

namespace catapult {
	namespace chain {
		class WeightedVotingCommitteeManager;
		class WeightedVotingCommitteeManagerV2;
	}
	namespace cache { class CommitteeAccountCollector; }
}

namespace catapult { namespace observers {

	/// Observes changes triggered by add harvester notifications
	DECLARE_OBSERVER(AddHarvester, model::AddHarvesterNotification<1>)();

	/// Observes changes triggered by remove harvester notifications
	DECLARE_OBSERVER(RemoveHarvester, model::RemoveHarvesterNotification<1>)();

	/// Observes changes triggered by block cosignatures notifications V1
	DECLARE_OBSERVER(UpdateHarvestersV1, model::BlockCommitteeNotification<1>)(
		const std::shared_ptr<chain::WeightedVotingCommitteeManager>& pCommitteeManager,
		const std::shared_ptr<cache::CommitteeAccountCollector>& pAccountCollector);

	/// Observes changes triggered by block cosignatures notifications V2
	DECLARE_OBSERVER(UpdateHarvestersV2, model::BlockCommitteeNotification<2>)(
		const std::shared_ptr<chain::WeightedVotingCommitteeManagerV2>& pCommitteeManager,
		const std::shared_ptr<cache::CommitteeAccountCollector>& pAccountCollector);

	/// Observes changes triggered by active harvesters notifications
	DECLARE_OBSERVER(ActiveHarvesters, model::ActiveHarvestersNotification<1>)();
}}
