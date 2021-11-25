/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "NotificationHandlers.h"

namespace catapult { namespace notification_handlers {

    using Notification = model::ReplicatorOffboardingNotification<1>;

    DECLARE_HANDLER(ReplicatorOffboarding, Notification)(const std::weak_ptr<storage::ReplicatorService>& pReplicatorServiceWeak) {
        return MAKE_HANDLER(ReplicatorOffboarding, [pReplicatorServiceWeak](const Notification& notification, const HandlerContext&) {
            auto pReplicatorService = pReplicatorServiceWeak.lock();
            if (!pReplicatorService)
                return;

            if (pReplicatorService->replicatorKey() == notification.PublicKey) {
                CATAPULT_LOG(debug) << "replicator off-boarding: stopping replicator service";
                pReplicatorService->terminate();
            }
        });
    }
}}
