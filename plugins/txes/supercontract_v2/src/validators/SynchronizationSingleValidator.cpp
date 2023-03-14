/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"

namespace catapult { namespace validators {

	using Notification = model::SynchronizationSingleNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(SynchronizationSingle, [](const Notification& notification, const ValidatorContext& context) {

		const auto& contractCache = context.Cache.sub<cache::SuperContractCache>();
		auto contractIt = contractCache.find(notification.ContractKey);

		// For the moment we should have already been checked that the contract exists
		const auto& contractEntry = contractIt.get();

		if (notification.BatchId + 1 != contractEntry.nextBatchId()) {
			return Failure_SuperContract_v2_Invalid_Batch_Id;
		}

		const auto& executorsInfo = contractEntry.executorsInfo();
		auto executorIt = executorsInfo.find(notification.Executor);

		if (executorIt == executorsInfo.end()) {
			return Failure_SuperContract_v2_Is_Not_Executor;
		}

		if (executorIt->second.NextBatchToApprove > notification.BatchId) {
			return Failure_SuperContract_v2_Batch_Already_Proven;
		}

		return ValidationResult::Success;
	})

}}
