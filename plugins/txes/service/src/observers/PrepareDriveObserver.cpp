/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "catapult/model/NetworkConfiguration.h"
#include "src/cache/DriveCache.h"

namespace catapult { namespace observers {

	DEFINE_OBSERVER(PrepareDrive, model::PrepareDriveNotification<1>, [](const auto& notification, const ObserverContext& context) {
		auto& driveCache = context.Cache.sub<cache::DriveCache>();
		if (NotifyMode::Commit == context.Mode) {
			state::DriveEntry driveEntry(notification.Drive);
			driveEntry.setStart(context.Height);
			driveEntry.setDuration(notification.Duration);
			driveEntry.setSize(notification.Size);
			driveEntry.setReplicas(notification.Replicas);
			state::DepositMap depositMap;
			depositMap.emplace(Hash256(), notification.Deposit);
			driveEntry.customers().emplace(notification.Customer, depositMap);
			driveCache.insert(driveEntry);
		} else {
			if (driveCache.contains(notification.Drive))
				driveCache.remove(notification.Drive);
		}
	});
}}
