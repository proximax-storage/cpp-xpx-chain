/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "NotificationHandlers.h"

namespace catapult { namespace notification_handlers {

	using Notification = model::BlockNotification<1>;

	DECLARE_HANDLER(ContractsUpdate, Notification)(const std::weak_ptr<contract::ExecutorService>& pExecutorServiceWeak) {
		return MAKE_HANDLER(ContractsUpdate, [pExecutorServiceWeak](const Notification& notification, const HandlerContext& context) {
			auto pExecutorService = pExecutorServiceWeak.lock();
			if (!pExecutorService) {
				return;
			}
			try {
				pExecutorService->updateContracts(context.Height);
			}
			catch (...) {
				CATAPULT_LOG(warning) << "An exception has occurred in the executor";
				pExecutorService->restart();
			}
		});
	}
}}