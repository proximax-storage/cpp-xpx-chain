/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"

namespace catapult { namespace observers {

	using Notification = model::ReplicatorOffboardingNotification<1>;
	using DrivePriority = std::pair<Key, double>;
	using DriveQueue = std::priority_queue<DrivePriority, std::vector<DrivePriority>, utils::DriveQueueComparator>;

	DECLARE_OBSERVER(ReplicatorOffboarding, Notification)(const std::shared_ptr<DriveQueue>& pDriveQueue) {
		return MAKE_OBSERVER(ReplicatorOffboarding, Notification, ([pDriveQueue](const Notification& notification, const ObserverContext& context) {
			if (NotifyMode::Rollback == context.Mode)
				CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (ReplicatorOffboarding)");

  			const auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::StorageConfiguration>();
			auto& driveCache = context.Cache.template sub<cache::BcDriveCache>();
		  	auto driveIter = driveCache.find(notification.DriveKey);
		  	auto& driveEntry = driveIter.get();

		  	driveEntry.offboardingReplicators().emplace(notification.PublicKey);

			if (driveEntry.replicators().size() < driveEntry.replicatorCount()) {
				DriveQueue originalQueue = *pDriveQueue;
				DriveQueue newQueue;
				newQueue.emplace(
						notification.DriveKey,
						utils::CalculateDrivePriority(driveEntry, pluginConfig.MinReplicatorCount));
				while (!originalQueue.empty()) {
					const auto drivePriorityPair = originalQueue.top();
					const auto& driveKey = drivePriorityPair.first;
					originalQueue.pop();

					if (driveKey != notification.DriveKey)
						newQueue.push(drivePriorityPair);
				}
				*pDriveQueue = std::move(newQueue);
			}
		}))
	}
}}