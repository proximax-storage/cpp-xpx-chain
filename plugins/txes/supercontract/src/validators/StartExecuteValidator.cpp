/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/cache/SuperContractCache.h"

namespace catapult { namespace validators {

	using Notification = model::StartExecuteNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(StartExecute, [](const Notification& notification, const StatefulValidatorContext& context) {
		const auto& superContractCache = context.Cache.sub<cache::SuperContractCache>();
		auto superContractCacheIter = superContractCache.find(notification.SuperContract);
		auto& superContractEntry = superContractCacheIter.get();

		if (std::numeric_limits<uint16_t>::max() == superContractEntry.executionCount())
			return Failure_SuperContract_Execution_Count_Exceeded_Limit;

		return ValidationResult::Success;
	});
}}
