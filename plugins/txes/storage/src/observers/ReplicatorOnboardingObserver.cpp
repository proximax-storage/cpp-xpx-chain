/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/utils/StorageUtils.h"

namespace catapult { namespace observers {

	using Notification = model::ReplicatorOnboardingNotification<1>;
	using DrivePriority = std::pair<Key, double>;
	using DriveQueue = std::priority_queue<DrivePriority, std::vector<DrivePriority>, utils::DriveQueueComparator>;

	DECLARE_OBSERVER(ReplicatorOnboarding, Notification)(const std::shared_ptr<DriveQueue>& pDriveQueue) {
		return MAKE_OBSERVER(ReplicatorOnboarding, Notification, ([pDriveQueue](const Notification& notification, const ObserverContext& context) {
			if (NotifyMode::Rollback == context.Mode)
				CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (ReplicatorOnboarding)");

			auto& replicatorCache = context.Cache.sub<cache::ReplicatorCache>();
			state::ReplicatorEntry replicatorEntry(notification.PublicKey);
			replicatorCache.insert(replicatorEntry);

			std::seed_seq seed(notification.Seed.begin(), notification.Seed.end());
		  	std::mt19937 rng(seed);
			utils::AssignReplicatorsToQueuedDrives({notification.PublicKey}, pDriveQueue, context, rng);
		}))
	}
}}
