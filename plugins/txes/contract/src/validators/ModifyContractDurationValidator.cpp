/**
*** Copyright 2018 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/cache/ContractCache.h"

namespace catapult { namespace validators {

	using Notification = model::ModifyContractNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(ModifyContractDuration, [](const auto& notification, const ValidatorContext& context) {
		const auto& contractCache = context.Cache.sub<cache::ContractCache>();
		if (!contractCache.contains(notification.Multisig) && (notification.DurationDelta <= 0))
            return Failure_Contract_Modify_Invalid_Duration;

		return ValidationResult::Success;
	});
}}
