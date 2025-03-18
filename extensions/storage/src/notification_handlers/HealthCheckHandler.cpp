/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "NotificationHandlers.h"
#include "catapult/utils/StorageUtils.h"

namespace catapult { namespace notification_handlers {

	using Notification = model::BlockNotification<1>;

	DECLARE_HANDLER(HealthCheck, Notification)(const std::weak_ptr<storage::ReplicatorService>& pReplicatorServiceWeak) {
		return MAKE_HANDLER(HealthCheck, [pReplicatorServiceWeak](const Notification& notification, const HandlerContext& context) {
			CATAPULT_LOG(warning) << "======================================================> replicator health check: start notification processing";
			auto pReplicatorService = pReplicatorServiceWeak.lock();
			if (!pReplicatorService) {
				CATAPULT_LOG(warning) << "======================================================> replicator health check: no replicator service";
				return;
			}

			pReplicatorService->post([](const auto& pReplicatorService) {
				if (!pReplicatorService->registered()) {
					CATAPULT_LOG(warning) << "======================================================> replicator health check: replicator is not registered";
					return;
				}

				if (!pReplicatorService->isAlive()) {
					CATAPULT_LOG(warning) << "======================================================> replicator health check: starting replicator";
					pReplicatorService->start();
				} else {
					CATAPULT_LOG(warning) << "======================================================> replicator health check: replicator is alive";
				}
			});
		});
	}
}}
