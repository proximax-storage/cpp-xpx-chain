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
		const auto& contractEntry = contractIt.get();

		if (notification.BatchId != contractEntry.nextBatchId()) {
			return Failure_SuperContract_Invalid_Batch_Id;
		}

		std::set<Key> cosigners;
		for (const auto& key: notification.Cosigners) {
			cosigners.insert(key);
		}

		if (cosigners.size() < notification.Cosigners.size()) {
			return Failure_SuperContract_Duplicate_Cosigner;
		}

		if (cosigners.size() <= 2 * contractEntry.executorsInfo().size() / 3) {
			return Failure_SuperContract_Not_Enough_Signatures;
		}

		return ValidationResult::Success;
	})

}}
