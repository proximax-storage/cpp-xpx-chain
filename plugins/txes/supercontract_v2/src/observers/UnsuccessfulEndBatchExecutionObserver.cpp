/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <random>
#include "Observers.h"

namespace catapult::observers {

	using Notification = model::UnsuccessfulBatchExecutionNotification<1>;

	DECLARE_OBSERVER(UnsuccessfulEndBatchExecution, Notification)() {
		return MAKE_OBSERVER(UnsuccessfulEndBatchExecution, Notification, ([](const Notification& notification, ObserverContext& context) {
			if (NotifyMode::Rollback == context.Mode)
				CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (UnsuccessfulBatchExecution)");

			auto& contractCache = context.Cache.sub<cache::SuperContractCache>();
			auto contractIt = contractCache.find(notification.ContractKey);
			auto& contractEntry = contractIt.get();
			auto& batch = (--contractEntry.batches().end())->second;
			batch.Success = false;
			batch.PoExVerificationInformation = crypto::CurvePoint();
		}))
	}
}
