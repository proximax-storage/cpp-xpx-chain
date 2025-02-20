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
		class WeightedVotingCommitteeManagerV3;
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

	/// Observes changes triggered by block cosignatures notifications V2
	DECLARE_OBSERVER(UpdateHarvestersV3, model::BlockCommitteeNotification<3>)(
		const std::shared_ptr<chain::WeightedVotingCommitteeManagerV3>& pCommitteeManager,
		const std::shared_ptr<cache::CommitteeAccountCollector>& pAccountCollector);

	/// Observes changes triggered by block cosignatures notifications V2
	DECLARE_OBSERVER(UpdateHarvestersV4, model::BlockCommitteeNotification<4>)(
		const std::shared_ptr<chain::WeightedVotingCommitteeManagerV3>& pCommitteeManager,
		const std::shared_ptr<cache::CommitteeAccountCollector>& pAccountCollector);

	/// Observes changes triggered by active harvesters notifications V1
	DECLARE_OBSERVER(ActiveHarvestersV1, model::ActiveHarvestersNotification<1>)();

	/// Observes changes triggered by active harvesters notifications V2
	DECLARE_OBSERVER(ActiveHarvestersV2, model::ActiveHarvestersNotification<2>)();

	/// Observes changes triggered by active harvesters notifications V3
	DECLARE_OBSERVER(ActiveHarvestersV3, model::ActiveHarvestersNotification<3>)();

	/// Observes changes triggered by active harvesters notifications V4
	DECLARE_OBSERVER(ActiveHarvestersV4, model::ActiveHarvestersNotification<4>)();

	/// Observes changes triggered by inactive harvesters notifications
	DECLARE_OBSERVER(InactiveHarvesters, model::InactiveHarvestersNotification<1>)();

	/// Observes changes triggered by remove DBRB process by network notifications
	DECLARE_OBSERVER(RemoveDbrbProcessByNetwork, model::RemoveDbrbProcessByNetworkNotification<1>)(
		const std::shared_ptr<cache::CommitteeAccountCollector>& pAccountCollector);
}}
