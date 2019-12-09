/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "CommonDrive.h"

namespace catapult { namespace observers {

	DEFINE_OBSERVER(JoinToDrive, model::JoinToDriveNotification<1>, [](const auto& notification, ObserverContext& context) {
		auto& driveCache = context.Cache.sub<cache::DriveCache>();
		auto driveIter = driveCache.find(notification.DriveKey);
		auto& driveEntry = driveIter.get();
		if (NotifyMode::Commit == context.Mode) {
			state::ReplicatorInfo info;
			info.Start = context.Height;

			// It is new replicator, so he doesn't have any files
			for (const auto& file : driveEntry.files())
				info.ActiveFilesWithoutDeposit.insert(file.first);

			driveEntry.replicators().emplace(notification.Replicator, info);

			if (driveEntry.replicators().size() >= driveEntry.minReplicators() && driveEntry.billingHistory().empty())
                SetDriveState(driveEntry, context, state::DriveState::Pending);
		} else {
			driveEntry.replicators().erase(notification.Replicator);
            if (driveEntry.replicators().size() < driveEntry.minReplicators() && driveEntry.billingHistory().empty())
                SetDriveState(driveEntry, context, state::DriveState::NotStarted);
		}

		UpdateDriveMultisigSettings(driveEntry, context);
	});
}}
