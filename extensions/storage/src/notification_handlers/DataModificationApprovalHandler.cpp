/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "NotificationHandlers.h"

namespace catapult { namespace notification_handlers {

    using Notification = model::DataModificationApprovalNotification<1>;

    DECLARE_HANDLER(DataModificationApproval, Notification)(const std::weak_ptr<storage::ReplicatorService>& pReplicatorServiceWeak) {
        return MAKE_HANDLER(DataModificationApproval, [pReplicatorServiceWeak](const Notification& notification, const HandlerContext&) {
            auto pReplicatorService = pReplicatorServiceWeak.lock();
            if (!pReplicatorService)
                return;

            auto replicatorsCount = notification.JudgedKeysCount + notification.OverlappingKeysCount + notification.JudgingKeysCount;
            std::vector<Key> replicators;
            replicators.reserve(replicatorsCount);

            for (auto i = 0; i < replicatorsCount; i++)
                replicators.emplace_back(notification.PublicKeysPtr[i]);

            pReplicatorService->dataModificationApprovalPublished(
                    notification.DriveKey,
                    notification.DataModificationId,
                    notification.FileStructureCdi,
                    replicators
            );
        });
    }
}}
