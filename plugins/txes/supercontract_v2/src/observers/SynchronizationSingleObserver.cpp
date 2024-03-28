/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <random>
#include "Observers.h"

namespace catapult::observers {

	using Notification = model::SynchronizationSingleNotification<1>;

	DECLARE_OBSERVER(SynchronizationSingle, Notification)(
			const std::unique_ptr<StorageExternalManagementObserver>& storageExternalManager) {
		return MAKE_OBSERVER(SynchronizationSingle, Notification, ([&storageExternalManager](const Notification& notification, ObserverContext& context) {
			if (NotifyMode::Rollback == context.Mode)
				CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (SynchronizationSingle)");

			auto& contractCache = context.Cache.sub<cache::SuperContractCache>();
			auto contractIt = contractCache.find(notification.ContractKey);
			auto& contractEntry = contractIt.get();

			auto& executor = contractEntry.executorsInfo()[notification.Executor];

			executor.NextBatchToApprove = contractEntry.nextBatchId();
			auto& poex = executor.PoEx;
			poex.StartBatchId = contractEntry.nextBatchId();
			poex.T = crypto::CurvePoint();
			poex.R = crypto::Scalar();

			storageExternalManager->addToConfirmedStorage(context, contractEntry.driveKey(), { notification.Executor });
		}))
	}
}
