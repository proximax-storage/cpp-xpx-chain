/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "NotificationHandlers.h"

namespace catapult { namespace notification_handlers {

	using Notification = model::EndDriveVerificationNotification<1>;

	DECLARE_HANDLER(EndDriveVerification, Notification)(const std::weak_ptr<storage::ReplicatorService>& pReplicatorServiceWeak) {
		return MAKE_HANDLER(FinishDownload, [pReplicatorServiceWeak](const Notification& notification, const HandlerContext& context) {
			auto pReplicatorService = pReplicatorServiceWeak.lock();
			if (!pReplicatorService)
				return;

			if (!pReplicatorService->driveExists(notification.DriveKey)) {
				pReplicatorService->updateReplicatorDownloadChannels();
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

//					pReplicatorService->endDriveVerificationPublished(
//							notification.DriveKey,
//							notification.VerificationTrigger);
				}
				else {
					// The transaction has already been processed when adding the Drive
				}
			}
			else if (assignedToDrive) {
				// We were added to Drive with the transaction
				pReplicatorService->addDrive(notification.DriveKey);
				pReplicatorService->updateDriveDownloadChannels(notification.DriveKey);
			}
			else if (driveAddedHeight) {
				// We were deleted from the Drive with the transaction
				pReplicatorService->removeDrive(notification.DriveKey);
				pReplicatorService->updateDriveDownloadChannels(notification.DriveKey);
				// In order to increase efficiency maybe it is needed to removed all channels and not update
			}
		});
	}
}}