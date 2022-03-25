/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "NotificationHandlers.h"
#include "src/catapult/crypto/Hashes.h"
#include <catapult/utils/StorageUtils.h>

namespace catapult { namespace notification_handlers {

	using Notification = model::BlockNotification<2>;

	DECLARE_HANDLER(PeriodicStoragePayment, Notification)(const std::weak_ptr<storage::ReplicatorService>& pReplicatorServiceWeak) {
		return MAKE_HANDLER(PeriodicStoragePayment, [pReplicatorServiceWeak](const Notification& notification, const HandlerContext& context) {
			auto pReplicatorService = pReplicatorServiceWeak.lock();
			if (!pReplicatorService)
				return;

			Hash256 eventHash = utils::getStoragePaymentEventHash(notification.Hash, context.Config.Immutable.GenerationHash);

			pReplicatorService->updateReplicatorDrives(eventHash);
			pReplicatorService->updateReplicatorDownloadChannels();
		});
	}
}}
