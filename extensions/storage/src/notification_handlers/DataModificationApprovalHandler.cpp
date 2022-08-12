/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "NotificationHandlers.h"

namespace catapult { namespace notification_handlers {

    using Notification = model::DataModificationApprovalNotification<1>;

    DECLARE_HANDLER(DataModificationApproval, Notification)(const std::weak_ptr<storage::ReplicatorService>& pReplicatorServiceWeak) {
        return MAKE_HANDLER(DataModificationApproval, [pReplicatorServiceWeak](const Notification& notification, const HandlerContext& context) {
            auto pReplicatorService = pReplicatorServiceWeak.lock();
            if (!pReplicatorService)
                return;

			if (!pReplicatorService->isAssignedToDrive(notification.DriveKey)) {
				// This case includes the situation when the drive does not already exist
				return;
			}

			auto driveAddedHeight = pReplicatorService->driveAddedAt(notification.DriveKey);

			if (!driveAddedHeight) {
				CATAPULT_LOG( error ) << "Replicator Is Assigned to Drive but it is not Added";
				return;
			}

			if (driveAddedHeight == context.Height) {
				// The approval has been already processed when adding the Drive
				return;
			}

			auto replicatorsCount = notification.JudgingKeysCount + notification.OverlappingKeysCount;
            std::vector<Key> replicators;
            replicators.reserve(replicatorsCount);

            for (auto i = 0; i < replicatorsCount; i++) {
				replicators.emplace_back(notification.PublicKeysPtr[i]);
			}

            pReplicatorService->dataModificationApprovalPublished(
                    notification.DriveKey,
                    notification.DataModificationId,
                    notification.FileStructureCdi,
                    replicators
            );
        });
    }
}}
