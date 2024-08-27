/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "NotificationHandlers.h"

namespace catapult { namespace notification_handlers {

	using Notification = model::StreamStartNotification<1>;

	DECLARE_HANDLER(StreamStart, Notification)(const std::weak_ptr<storage::ReplicatorService>& pReplicatorServiceWeak) {
		return MAKE_HANDLER(StreamStart, [pReplicatorServiceWeak](const Notification& notification, const HandlerContext& context) {
			auto pReplicatorService = pReplicatorServiceWeak.lock();
			if (!pReplicatorService) {
				return;
			}

			// If we do not perform this check, we will try to read from database non-existing information
			if (!pReplicatorService->driveExists(notification.DriveKey, context.Cache)) {
				// During the modification Replicator could be assigned
				// to some download channels of the already non-existing drive
				pReplicatorService->updateReplicatorDownloadChannels(context.Cache);
				pReplicatorService->maybeRestart(context.Cache);
				return;
			}

			auto driveAddedHeight = pReplicatorService->driveAddedAt(notification.DriveKey);
			bool assignedToDrive = pReplicatorService->isAssignedToDrive(notification.DriveKey, context.Cache);

			if (assignedToDrive && driveAddedHeight) {
				if(driveAddedHeight < context.Height) {
					// Drive Replicators Can be Changed with this transaction
					pReplicatorService->updateDriveReplicators(notification.DriveKey, context.Cache);
					pReplicatorService->updateShardDonator(notification.DriveKey, context.Cache);
					pReplicatorService->updateShardRecipient(notification.DriveKey, context.Cache);

					pReplicatorService->updateDriveDownloadChannels(notification.DriveKey, context.Cache);

					pReplicatorService->startStream(
							notification.DriveKey,
							notification.StreamId,
							notification.Owner,
							notification.FolderName,
							notification.ExpectedUploadSize,
							context.Cache);
				}
				else {
					// The modification has already been processed when adding the Drive
				}
			}
			else if (assignedToDrive) {
				// We were added to Drive with the transaction
				pReplicatorService->addDrive(notification.DriveKey, context.Cache);
				pReplicatorService->updateDriveDownloadChannels(notification.DriveKey, context.Cache);
			}
			else if (driveAddedHeight) {
				// We were deleted from the Drive with the transaction
				pReplicatorService->removeDrive(notification.DriveKey);
				pReplicatorService->updateDriveDownloadChannels(notification.DriveKey, context.Cache);
				// In order to increase efficiency maybe it is needed to remove all channels and not update
			}
			pReplicatorService->maybeRestart(context.Cache);
		});
	}
}}
