/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "NotificationHandlers.h"
#include "src/catapult/crypto/Hashes.h"

namespace catapult { namespace notification_handlers {

	using Notification = model::BlockNotification<2>;

	DECLARE_HANDLER(DownloadChannelPayment, Notification)(const std::weak_ptr<storage::ReplicatorService>& pReplicatorServiceWeak) {
		return MAKE_HANDLER(DownloadChannelPayment, [pReplicatorServiceWeak](const Notification& notification, const HandlerContext& context) {
			auto pReplicatorService = pReplicatorServiceWeak.lock();
			if (!pReplicatorService)
				return;

			Hash256 eventHash;
			crypto::Sha3_256_Builder sha3;
			const std::string salt = "Download";
			sha3.update({notification.Hash,
						 utils::RawBuffer(reinterpret_cast<const uint8_t*>(salt.data()), salt.size()),
						 context.Config.Immutable.GenerationHash});
			sha3.final(eventHash);

			pReplicatorService->downloadBlockPublished(notification.Hash);
		});
	}
}}
