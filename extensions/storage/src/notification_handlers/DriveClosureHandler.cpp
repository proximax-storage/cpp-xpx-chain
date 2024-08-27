/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "NotificationHandlers.h"

namespace catapult { namespace notification_handlers {

	using Notification = model::DriveClosureNotification<1>;

	DECLARE_HANDLER(DriveClosure, Notification)(const std::weak_ptr<storage::ReplicatorService>& pReplicatorServiceWeak) {
		return MAKE_HANDLER(DriveClosure, [pReplicatorServiceWeak](const Notification& notification, const HandlerContext& context) {
			auto pReplicatorService = pReplicatorServiceWeak.lock();
			if (!pReplicatorService)
				return;

			auto addedToDriveHeight = pReplicatorService->driveAddedAt(notification.DriveKey);

			if (addedToDriveHeight) {
				// Note that Drive does not already exist in Cache
				pReplicatorService->closeDrive(notification.DriveKey, notification.TransactionHash);
			}

			// We could be assigned to the Drive in the same block, so in any case we try to explore new Drives.
			// It can be optimized if the cache stores all former Drives of the Replicator
			pReplicatorService->exploreNewReplicatorDrives(context.Cache);
			pReplicatorService->maybeRestart(context.Cache);
		});
	}
}}