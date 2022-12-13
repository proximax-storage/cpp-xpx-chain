/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"

namespace catapult { namespace validators {

	using Notification = model::ContractStateUpdateNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(ContractStateUpdateNotification, [](const Notification& notification, const ValidatorContext& context) {

		const auto& contractCache = context.Cache.sub<cache::SuperContractCache>();

		auto contractIt = contractCache.find(notification.ContractKey);
		const auto* contractEntry = contractIt.tryGet();

		if (!contractEntry) {
			return Failure_SuperContract_Contract_Does_Not_Exist;
		}

		return ValidationResult::Success;
	})

}}
