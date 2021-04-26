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
			state::DriveEntry driveEntry(notification.DriveKey);
            driveEntry.setOwner(notification.Owner);
			driveEntry.setSize(notification.DriveSize.unwrap());
			driveEntry.setReplicatorCount(notification.ReplicatorCount);
			driveCache.insert(driveEntry);
		} else {
			if (driveCache.contains(notification.DriveKey))
				driveCache.remove(notification.DriveKey);
		}
	});
}}
