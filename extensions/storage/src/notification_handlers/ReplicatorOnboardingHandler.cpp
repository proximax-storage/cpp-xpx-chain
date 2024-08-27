/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "NotificationHandlers.h"

namespace catapult { namespace notification_handlers {

	namespace {
		template<typename TNotification>
		void HandleReplicatorOnboarding(const std::weak_ptr<storage::ReplicatorService>& pReplicatorServiceWeak, const TNotification& notification, const HandlerContext& context) {
			auto pReplicatorService = pReplicatorServiceWeak.lock();
			if (!pReplicatorService)
				return;

			if (pReplicatorService->replicatorKey() == notification.PublicKey || !pReplicatorService->isReplicatorRegistered(notification.PublicKey, context.Cache)) {
				CATAPULT_LOG(debug) << "replicator on-boarding: starting replicator service";
				pReplicatorService->start(context.Cache);
			}
			else {
				// Replicator could be assigned to some of our Drives, Download Channels, Shards
				pReplicatorService->anotherReplicatorOnboarded(notification.PublicKey, context.Cache);
				pReplicatorService->maybeRestart(context.Cache);
			}
		}
	}

	DECLARE_HANDLER(ReplicatorOnboardingV1, model::ReplicatorOnboardingNotification<1>)(const std::weak_ptr<storage::ReplicatorService>& pReplicatorServiceWeak) {
		return MAKE_HANDLER_WITH_TYPE(ReplicatorOnboardingV1, model::ReplicatorOnboardingNotification<1>, [pReplicatorServiceWeak](const model::ReplicatorOnboardingNotification<1>& notification, const HandlerContext& context) {
			HandleReplicatorOnboarding(pReplicatorServiceWeak, notification, context);
		});
	}

	DECLARE_HANDLER(ReplicatorOnboardingV2, model::ReplicatorOnboardingNotification<2>)(const std::weak_ptr<storage::ReplicatorService>& pReplicatorServiceWeak) {
		return MAKE_HANDLER_WITH_TYPE(ReplicatorOnboardingV2, model::ReplicatorOnboardingNotification<2>, [pReplicatorServiceWeak](const model::ReplicatorOnboardingNotification<2>& notification, const HandlerContext& context) {
			HandleReplicatorOnboarding(pReplicatorServiceWeak, notification, context);
		});
	}
}}
