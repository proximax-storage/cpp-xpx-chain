/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "NotificationHandlers.h"

namespace catapult { namespace notification_handlers {

	using Notification = model::DownloadNotification<1>;

	DECLARE_HANDLER(Download, Notification)(const std::weak_ptr<storage::ReplicatorService>& pReplicatorServiceWeak) {
		return MAKE_HANDLER(Download, [pReplicatorServiceWeak](const Notification& notification, const HandlerContext&) {
			auto pReplicatorService = pReplicatorServiceWeak.lock();
			if (!pReplicatorService)
				return;

			std::vector<Key> listOfPublicKeys;
			auto pKey = notification.WhitelistedPublicKeysPtr;
		  	for (auto i = 0u; i < notification.WhitelistedPublicKeyCount; ++pKey, ++i)
				listOfPublicKeys.push_back(*pKey);

			pReplicatorService->addConsumer(notification.Consumer, std::move(listOfPublicKeys), notification.DownloadSize);
		});
	}
}}
