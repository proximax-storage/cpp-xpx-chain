/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "NotificationHandlers.h"

namespace catapult { namespace notification_handlers {

	using Notification = model::StreamFinishNotification<1>;

	DECLARE_HANDLER(StreamFinish, Notification)(const std::weak_ptr<storage::ReplicatorService>& pReplicatorServiceWeak) {
		return MAKE_HANDLER(StreamFinish, [pReplicatorServiceWeak](const Notification& notification, const HandlerContext& context) {
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
				// The cancel has been already taken into account when adding the Drive
				return;
			}

			pReplicatorService->finishStream(
					notification.DriveKey,
					notification.StreamId,
					notification.StreamStructureCdi,
					notification.ActualUploadSize);
			pReplicatorService->maybeRestart();
		});
	}
}}
