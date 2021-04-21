/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "CommonDrive.h"

namespace catapult { namespace observers {

	DEFINE_OBSERVER(PrepareDrive, model::PrepareDriveNotification<1>, [](const model::PrepareDriveNotification<1>& notification, ObserverContext& context) {
		auto& driveCache = context.Cache.sub<cache::DriveCache>();
		if (NotifyMode::Commit == context.Mode) {
			// TODO: Check driveEntry creation
			state::DriveEntry driveEntry(notification.DriveKey);
            SetDriveState(driveEntry, context, state::DriveState::NotStarted);
			driveEntry.setOwner(notification.Owner);
			driveEntry.setStart(context.Height);
			/**/ driveEntry.setDuration(BlockDuration()); // notification.Duration
			/**/ driveEntry.setBillingPeriod(BlockDuration()); // notification.BillingPeriod
			/**/ driveEntry.setBillingPrice(Amount()); // notification.BillingPrice
			driveEntry.setSize(notification.DriveSize.unwrap());
			driveEntry.setReplicas(notification.ReplicatorCount);
			/**/ driveEntry.setMinReplicators(0); // notification.MinReplicators
			/**/ driveEntry.setPercentApprovers(0); // notification.PercentApprovers
			driveCache.insert(driveEntry);
		} else {
			if (driveCache.contains(notification.DriveKey))
				driveCache.remove(notification.DriveKey);
		}
	});
}}
