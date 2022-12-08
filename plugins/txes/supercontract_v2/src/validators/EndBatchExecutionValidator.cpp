/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"

namespace catapult { namespace validators {

	using Notification = model::EndBatchExecutionNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(EndBatchExecution, [](const Notification& notification, const ValidatorContext& context) {

		const auto& contractCache = context.Cache.sub<cache::SuperContractCache>();

		auto contractIt = contractCache.find(notification.ContractKey);
		auto* pContractEntry = contractIt.tryGet();

		if (!pContractEntry) {
			return Failure_SuperContract_Contract_Does_Not_Exist;
		}

		if (notification.BatchId != pContractEntry->nextBatchId()) {
			return Failure_SuperContract_Invalid_Batch_Id;
		}

		return ValidationResult::Success;
	})

}}
