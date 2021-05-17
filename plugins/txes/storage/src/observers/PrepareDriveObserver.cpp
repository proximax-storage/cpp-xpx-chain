/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"

namespace catapult { namespace observers {

	DEFINE_OBSERVER(PrepareDrive, model::PrepareDriveNotification<1>, [](const model::PrepareDriveNotification<1>& notification, ObserverContext& context) {
		if (NotifyMode::Rollback == context.Mode)
			CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (PrepareDrive)");

	  	auto& driveCache = context.Cache.sub<cache::BcDriveCache>();
		state::BcDriveEntry driveEntry(notification.DriveKey);
		driveEntry.setOwner(notification.Owner);
		driveEntry.setSize(notification.DriveSize);
		driveEntry.setReplicatorCount(notification.ReplicatorCount);
		driveCache.insert(driveEntry);
	});
}}
