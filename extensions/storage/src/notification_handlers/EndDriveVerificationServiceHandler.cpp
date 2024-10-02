/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "NotificationHandlers.h"

namespace catapult { namespace notification_handlers {

	using Notification = model::EndDriveVerificationServiceNotification<1>;

	DECLARE_HANDLER(EndDriveVerificationService, Notification)(const std::weak_ptr<storage::ReplicatorService>& pReplicatorServiceWeak) {
		return MAKE_HANDLER(EndDriveVerificationService, [pReplicatorServiceWeak](const Notification& notification, const HandlerContext& context) {
			auto pReplicatorService = pReplicatorServiceWeak.lock();
			if (!pReplicatorService)
				return;

			pReplicatorService->post([notification](const auto& pReplicatorService) {
				if (!pReplicatorService->initialized())
					return;

				if (!pReplicatorService->isAlive()) {
					pReplicatorService->stop();
					return;
				}

				bool driveAdded = pReplicatorService->driveAdded(notification.DrivePtr->Id);
				const auto& replicators = notification.DrivePtr->Replicators;
				bool assignedToDrive = (replicators.find(pReplicatorService->replicatorKey()) != replicators.cend());
				if (assignedToDrive && driveAdded) {
					// Drive replicators could change with this transaction
					pReplicatorService->updateDrive(notification.DrivePtr);
					if (notification.Replicators.find(pReplicatorService->replicatorKey()) != notification.Replicators.cend())
						pReplicatorService->endDriveVerificationPublished(notification.DrivePtr->Id, notification.VerificationTrigger);
				} else if (assignedToDrive) {
					// This replicator was added to the drive with the transaction
					pReplicatorService->addDrive(notification.DrivePtr);
				} else if (driveAdded) {
					// This replicator was removed from the drive with the transaction
					pReplicatorService->removeDrive(notification.DrivePtr->Id);
				}
			});
		});
	}
}}