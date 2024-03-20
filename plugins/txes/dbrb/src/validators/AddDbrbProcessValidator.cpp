/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/cache/DbrbViewCache.h"


namespace catapult { namespace validators {

	using Notification = model::AddDbrbProcessNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(AddDbrbProcess, ([](const Notification& notification, const ValidatorContext& context) {
		const auto& cache = context.Cache.sub<cache::DbrbViewCache>();
		auto iter = cache.find(notification.ProcessId);
		auto pEntry = iter.tryGet();
		if (pEntry && context.BlockTime < pEntry->expirationTime() - Timestamp(context.Config.Network.DbrbRegistrationGracePeriod.millis()) )
			return Failure_Dbrb_Process_Not_Expired;

		return ValidationResult::Success;
	}));
}}
