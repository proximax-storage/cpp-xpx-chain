/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "NotificationHandlers.h"

namespace catapult { namespace notification_handlers {

	using Notification = model::DownloadNotification<1>;

	DECLARE_HANDLER(Download, Notification)(const std::weak_ptr<storage::ReplicatorService>& pReplicatorServiceWeak) {
		return MAKE_HANDLER(Download, [pReplicatorServiceWeak](const Notification& notification, const HandlerContext& context) {
			auto pReplicatorService = pReplicatorServiceWeak.lock();
			if (!pReplicatorService)
				return;

			auto channelAddedHeight = pReplicatorService->channelAddedAt(notification.Id);

			if (channelAddedHeight) {
				// The channel creation has already been processed when starting the Replicator
				if (*channelAddedHeight != context.Height) {
					CATAPULT_LOG( error ) << "Channel Added Before its Creation";
				}
				return;
			}

			if(pReplicatorService->isAssignedToChannel(notification.Id)) {
				pReplicatorService->addDownloadChannel(notification.Id);
				pReplicatorService->maybeRestart();
			}
		});
	}
}}
