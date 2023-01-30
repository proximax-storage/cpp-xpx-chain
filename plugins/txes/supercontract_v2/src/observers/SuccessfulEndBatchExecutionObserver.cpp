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
			auto& batch = contractEntry.batches().back();
			batch.Success = true;
			batch.PoExVerificationInformation = notification.VerificationInformation;

			crypto::Sha3_256_Builder hashBuilder;
			hashBuilder.update(notification.ContractKey);
			hashBuilder.update(utils::RawBuffer{reinterpret_cast<const uint8_t*>(notification.BatchId), sizeof(notification.BatchId)});
			Hash256 modificationId;
			hashBuilder.final(modificationId);

			if (notification.UpdateStorageState) {
				storageExternalManager->updateStorageState(context,
														   contractEntry.driveKey(),
														   notification.StorageHash,
														   modificationId,
														   notification.UsedSizeBytes,
														   notification.MetaFilesSizeBytes);
			}
		}))
	}
}
