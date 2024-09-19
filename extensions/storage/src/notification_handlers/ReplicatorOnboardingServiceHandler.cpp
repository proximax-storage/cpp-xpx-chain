/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "NotificationHandlers.h"

namespace catapult { namespace notification_handlers {

    using Notification = model::ReplicatorOnboardingServiceNotification<1>;

    DECLARE_HANDLER(ReplicatorOnboardingService, Notification)(const std::weak_ptr<storage::ReplicatorService>& pReplicatorServiceWeak) {
        return MAKE_HANDLER(ReplicatorOnboardingService, [pReplicatorServiceWeak](const Notification& notification, const HandlerContext&) {
			CATAPULT_LOG(warning) << "======================================================> replicator on-boarding: start notification processing";
			auto pReplicatorService = pReplicatorServiceWeak.lock();
			if (!pReplicatorService) {
				CATAPULT_LOG(warning) << "======================================================> replicator on-boarding: no replicator service";
				return;
			}

			if (pReplicatorService->replicatorKey() != notification.ReplicatorKey) {
				CATAPULT_LOG(warning) << "======================================================> replicator on-boarding: replicator key mismatch";
				return;
			}

			pReplicatorService->post([notification](const auto& pReplicatorService) {
				CATAPULT_LOG(warning) << "======================================================> replicator on-boarding: starting replicator service";
				pReplicatorService->markRegistered();
			});
        });
    }
}}
