/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "NotificationHandlers.h"

namespace catapult { namespace notification_handlers {

	using Notification = model::AutomaticExecutionsReplenishmentNotification<1>;

	DECLARE_HANDLER(AutomaticExecutionsReplenishment, Notification)(const std::weak_ptr<contract::ExecutorService>& pExecutorServiceWeak) {
		return MAKE_HANDLER(AutomaticExecutionsReplenishment, [pExecutorServiceWeak](const Notification& notification, const HandlerContext& context) {
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

				pExecutorService->activateAutomaticExecutions(notification.ContractKey, context.Height);
			}
			catch (...) {

			}
		});
	}
}}