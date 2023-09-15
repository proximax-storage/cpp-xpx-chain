/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "NotificationHandlers.h"

namespace catapult { namespace notification_handlers {

	using Notification = model::SynchronizationSingleNotification<1>;

	DECLARE_HANDLER(SynchronizeSingle, Notification)(const std::weak_ptr<contract::ExecutorService>& pExecutorServiceWeak) {
		return MAKE_HANDLER(SynchronizeSingle, [pExecutorServiceWeak](const Notification& notification, const HandlerContext& context) {
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

				if (notification.Executor != pExecutorService->executorKey()) {
					return;
				}

//				pExecutorService->updateConfig();
				pExecutorService->synchronizeSinglePublished(notification.ContractKey, notification.BatchId);
			}
			catch (...) {
				CATAPULT_LOG(warning) << "An exception has occurred in the executor";
				pExecutorService->restart();
			}
		});
	}
}}