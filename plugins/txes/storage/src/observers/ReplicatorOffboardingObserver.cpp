/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"

namespace catapult { namespace observers {

	using Notification = model::ReplicatorOffboardingNotification<1>;

	DECLARE_OBSERVER(ReplicatorOffboarding, Notification)() {
		return MAKE_OBSERVER(ReplicatorOffboarding, Notification, ([](const Notification& notification, const ObserverContext& context) {
			if (NotifyMode::Rollback == context.Mode)
				CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (ReplicatorOffboarding)");

  			const auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::StorageConfiguration>();
			auto& driveCache = context.Cache.template sub<cache::BcDriveCache>();
		  	auto driveIter = driveCache.find(notification.DriveKey);
		  	auto& driveEntry = driveIter.get();

		  	driveEntry.offboardingReplicators().emplace_back(notification.PublicKey);

			if (driveEntry.replicators().size() < driveEntry.replicatorCount()) {
				auto& priorityQueueCache = context.Cache.sub<cache::PriorityQueueCache>();
				auto driveQueueIt = getPriorityQueueIter(priorityQueueCache, state::DrivePriorityQueueKey);
				auto& driveQueueEntry = driveQueueIt.get();
				const auto newPriority = utils::CalculateDrivePriority(driveEntry, pluginConfig.MinReplicatorCount);

				driveQueueEntry.set(notification.DriveKey, newPriority);
			}
		}))
	}
}}