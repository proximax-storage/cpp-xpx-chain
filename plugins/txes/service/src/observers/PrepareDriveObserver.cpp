/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "CommonDrive.h"

namespace catapult { namespace observers {

	DEFINE_OBSERVER(PrepareDrive, model::PrepareDriveNotification<1>, [](const auto& notification, ObserverContext& context) {
		auto& driveCache = context.Cache.sub<cache::DriveCache>();
		if (NotifyMode::Commit == context.Mode) {
			state::DriveEntry driveEntry(notification.DriveKey);
            SetDriveState(driveEntry, context, state::DriveState::NotStarted);
			driveEntry.setOwner(notification.Owner);
			driveEntry.setDuration(notification.Duration);
			driveEntry.setBillingPeriod(notification.BillingPeriod);
			driveEntry.setBillingPrice(notification.BillingPrice);
			driveEntry.setSize(notification.DriveSize);
			driveEntry.setReplicas(notification.Replicas);
			driveEntry.setMinReplicators(notification.MinReplicators);
			driveEntry.setPercentApprovers(notification.PercentApprovers);
			driveCache.insert(driveEntry);
		} else {
			if (driveCache.contains(notification.DriveKey))
				driveCache.remove(notification.DriveKey);
		}
	});
}}
