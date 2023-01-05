/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "NotificationHandlers.h"

namespace catapult { namespace notification_handlers {

	using Notification = model::BatchExecutionSingleNotification<1>;

	DECLARE_HANDLER(BatchExecutionSingle, Notification)(const std::weak_ptr<contract::ExecutorService>& pExecutorServiceWeak) {
		return MAKE_HANDLER(BatchExecutionSingle, [pExecutorServiceWeak](const Notification& notification, const HandlerContext& context) {
			auto pExecutorService = pExecutorServiceWeak.lock();
			if (!pExecutorService) {
				return;
			}

			try {
				if (!pExecutorService->contractExists(notification.ContractKey)) {
					return;
				}

				auto contractAddedAt = pExecutorService->contractAddedAt(notification.ContractKey);
				if (!contractAddedAt || contractAddedAt >= context.Height) {
					return;
				}

				if (notification.Signer != pExecutorService->executorKey()) {
					return;
				}

				pExecutorService->batchExecutionSinglePublished(notification.ContractKey, notification.BatchId);
			}
			catch (...) {

			}
		});
	}
}}