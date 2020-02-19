/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/cache/SuperContractCache.h"
#include "catapult/validators/ValidatorContext.h"
#include <unordered_set>

namespace catapult { namespace validators {

	using Notification = model::SuperContractNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(SuperContract, [](const Notification& notification, const ValidatorContext& context) {
		const auto& superContractCache = context.Cache.sub<cache::SuperContractCache>();

		if (!superContractCache.contains(notification.SuperContractKey))
			return Failure_SuperContract_SuperContract_Is_Not_Exist;

		return ValidationResult::Success;
	});
}}
