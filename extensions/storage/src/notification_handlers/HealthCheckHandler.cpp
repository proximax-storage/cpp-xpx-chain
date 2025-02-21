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
			auto pReplicatorService = pReplicatorServiceWeak.lock();
			if (!pReplicatorService)
				return;

			pReplicatorService->post([](const auto& pReplicatorService) {
				if (!pReplicatorService->registered())
					return;

				if (!pReplicatorService->isAlive())
					pReplicatorService->start();
			});
		});
	}
}}
