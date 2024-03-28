/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/model/SupercontractNotifications.h"
#include "catapult/notification_handlers/HandlerContext.h"
#include "catapult/notification_handlers/NotificationHandlerTypes.h"
#include "../ExecutorService.h"

namespace catapult { namespace notification_handlers {

	DECLARE_HANDLER(ManualCall, model::ManualCallNotification<1>)(const std::weak_ptr<contract::ExecutorService>& pExecutorService);

	DECLARE_HANDLER(AutomaticExecutionsReplenishment, model::AutomaticExecutionsReplenishmentNotification<1>)(const std::weak_ptr<contract::ExecutorService>& pExecutorService);

	DECLARE_HANDLER(SuccessfulBatchExecution, model::SuccessfulBatchExecutionNotification<1>)(const std::weak_ptr<contract::ExecutorService>& pExecutorService);

	DECLARE_HANDLER(UnsuccessfulBatchExecution, model::UnsuccessfulBatchExecutionNotification<1>)(const std::weak_ptr<contract::ExecutorService>& pExecutorService);

	DECLARE_HANDLER(BatchExecutionSingle, model::BatchExecutionSingleNotification<1>)(const std::weak_ptr<contract::ExecutorService>& pExecutorService);

	DECLARE_HANDLER(SynchronizeSingle, model::SynchronizationSingleNotification<1>)(const std::weak_ptr<contract::ExecutorService>& pExecutorService);

	DECLARE_HANDLER(AutomaticExecutionsBlock, model::BlockNotification<1>)(const std::weak_ptr<contract::ExecutorService>& pExecutorService);

	DECLARE_HANDLER(ContractsUpdate, model::BlockNotification<1>)(const std::weak_ptr<contract::ExecutorService>& pExecutorService);
}}
