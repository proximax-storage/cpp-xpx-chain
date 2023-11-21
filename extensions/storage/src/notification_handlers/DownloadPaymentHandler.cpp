/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "NotificationHandlers.h"

namespace catapult { namespace notification_handlers {

	using Notification = model::DownloadPaymentNotification<1>;

	DECLARE_HANDLER(DownloadPayment, Notification)(const std::weak_ptr<storage::ReplicatorService>& pReplicatorServiceWeak) {
		return MAKE_HANDLER(DownloadPayment, [pReplicatorServiceWeak](const Notification& notification, const HandlerContext& context) {
			auto pReplicatorService = pReplicatorServiceWeak.lock();
			if (!pReplicatorService)
				return;

			if(!pReplicatorService->isAssignedToChannel(notification.DownloadChannelId)) {
				return;
			}

			auto channelAddedHeight = pReplicatorService->channelAddedAt(notification.DownloadChannelId);

			if (!channelAddedHeight) {
				CATAPULT_LOG( error ) << "Replicator Is Assigned to Channel but it is not Added";
				return;
			}

			if (channelAddedHeight == context.Height) {
				// The increase has been already taken into account when adding the Channel
				return;
			}

			pReplicatorService->increaseDownloadChannelSize(notification.DownloadChannelId);
			pReplicatorService->maybeRestart();
		});
	}
}}
