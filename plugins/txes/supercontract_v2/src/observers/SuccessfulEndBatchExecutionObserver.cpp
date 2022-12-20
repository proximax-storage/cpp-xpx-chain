/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <random>
#include "Observers.h"

namespace catapult::observers {

	using Notification = model::SuccessfulBatchExecutionNotification<1>;

	DECLARE_OBSERVER(SuccessfulEndBatchExecution, Notification)(const std::unique_ptr<StorageExternalManagementObserver>& storageExternalManager) {
		return MAKE_OBSERVER(SuccessfulEndBatchExecution, Notification, ([&storageExternalManager](const Notification& notification, ObserverContext& context) {
			if (NotifyMode::Rollback == context.Mode)
				CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (SuccessfulBatchExecution)");

			auto& contractCache = context.Cache.sub<cache::SuperContractCache>();
			auto contractIt = contractCache.find(notification.ContractKey);
			auto& contractEntry = contractIt.get();
			contractEntry.batches().back().PoExVerificationInformation = notification.VerificationInformation;

			if (notification.UpdateStorageState) {
				storageExternalManager->updateStorageState(context,
														   contractEntry.driveKey(),
														   notification.StorageHash,
														   notification.UsedSizeBytes,
														   notification.MetaFilesSizeBytes);
			}
		}))
	}
}
