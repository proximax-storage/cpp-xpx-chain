/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "NotificationHandlers.h"

namespace catapult { namespace notification_handlers {

	using Notification = model::ReplicatorOnboardingNotification<1>;

	DECLARE_HANDLER(ReplicatorOnboarding, Notification)(const std::weak_ptr<storage::ReplicatorService>& pReplicatorServiceWeak) {
		return MAKE_HANDLER(ReplicatorOnboarding, [pReplicatorServiceWeak](const Notification& notification, const HandlerContext&) {
			auto pReplicatorService = pReplicatorServiceWeak.lock();
			if (!pReplicatorService)
				return;

			if (pReplicatorService->replicatorKey() != notification.PublicKey || !pReplicatorService->isReplicatorRegistered(notification.PublicKey))
				return;

			CATAPULT_LOG(debug) << "replicator on-boarding: starting replicator service";
			pReplicatorService->start();
		});
	}
}}
