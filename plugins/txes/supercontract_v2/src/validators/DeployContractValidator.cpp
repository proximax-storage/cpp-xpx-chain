/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"

namespace catapult { namespace validators {

	using Notification = model::DeploySupercontractNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(DeployContract, [](const Notification& notification, const ValidatorContext& context) {

		const auto& driveCache = context.Cache.sub<cache::DriveContractCache>();

		if (driveCache.contains(notification.DriveKey)) {
			return Failure_SuperContract_v2_Contract_Already_Deployed_On_Drive;
		}

		return ValidationResult::Success;
	})

}}
