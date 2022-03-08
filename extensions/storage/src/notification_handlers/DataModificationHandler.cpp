/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "NotificationHandlers.h"

namespace catapult { namespace notification_handlers {

	using Notification = model::DataModificationNotification<1>;

	DECLARE_HANDLER(DataModification, Notification)(const std::weak_ptr<storage::ReplicatorService>& pReplicatorServiceWeak) {
		return MAKE_HANDLER(DataModification, [pReplicatorServiceWeak](const Notification& notification, const HandlerContext& context) {
			auto pReplicatorService = pReplicatorServiceWeak.lock();
			if (!pReplicatorService)
				return;

			if (!pReplicatorService->driveExists(notification.DriveKey)) {
				// The drive does not already exist so we are not inerested in its modifications and replicators changes
				return;
			}

			auto driveAddedHeight = pReplicatorService->driveAddedAt(notification.DriveKey);
			bool assignedToDrive = pReplicatorService->isAssignedToDrive(notification.DriveKey);

			if (assignedToDrive && driveAddedHeight) {
				if(driveAddedHeight < context.Height) {
					// Drive Replicators Can be Changed with this transaction
					pReplicatorService->updateDriveReplicators(notification.DriveKey);
					pReplicatorService->updateShardDonator(notification.DriveKey);
					pReplicatorService->updateShardRecipient(notification.DriveKey);

					pReplicatorService->updateDriveDownloadChannels(notification.DriveKey);

					pReplicatorService->addDriveModification(
							notification.DriveKey,
							notification.DownloadDataCdi,
							notification.DataModificationId,
							notification.Owner,
							notification.UploadSizeMegabytes);
				}
				else {
					// The modification has already been processed when adding the Drive
				}
			}
			else if (assignedToDrive) {
				// We were added to Drive with the transaction
				pReplicatorService->addDrive(notification.DriveKey);
			}
			else if (driveAddedHeight) {
				// We were deleted from the Drive with the transaction
				pReplicatorService->removeDrive(notification.DriveKey);
			}
			else if (!pReplicatorService->driveExists(notification.DriveKey)){
				// Actually, it is possible that we were added to the drive with the transaction
				// but were removed from it in the same block.
				// Such a situation is ignored when working with Drives
				// but requires correct closing Download Channels if needed so we can not fully ignore it
			}
		});
	}
}}
