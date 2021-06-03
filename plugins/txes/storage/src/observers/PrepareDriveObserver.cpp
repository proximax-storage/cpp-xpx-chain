/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"

namespace catapult { namespace observers {

	using Notification = model::PrepareDriveNotification<1>;

	DECLARE_OBSERVER(PrepareDrive, Notification)(const std::shared_ptr<cache::ReplicatorKeyCollector>& pKeyCollector) {
		return MAKE_OBSERVER(PrepareDrive, Notification, [pKeyCollector](const Notification& notification, const ObserverContext& context) {
			if (NotifyMode::Rollback == context.Mode)
				CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (PrepareDrive)");

			auto& driveCache = context.Cache.sub<cache::BcDriveCache>();
			state::BcDriveEntry driveEntry(notification.DriveKey);
			driveEntry.setOwner(notification.Owner);
			driveEntry.setSize(notification.DriveSize);
			driveEntry.setReplicatorCount(notification.ReplicatorCount);
			driveCache.insert(driveEntry);

			auto& replicatorCache = context.Cache.sub<cache::ReplicatorCache>();
			auto replicatorIter = replicatorCache.find(*pKeyCollector->keys().begin());
			auto& replicatorEntry = replicatorIter.get();
			replicatorEntry.drives().emplace_back(notification.DriveKey);
		})
	}
}}
