/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "catapult/validators/ValidatorContext.h"
#include "plugins/txes/multisig/src/cache/MultisigCache.h"
#include "src/cache/DriveCache.h"

namespace catapult { namespace validators {

	using Notification = model::PrepareDriveNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(PrepareDrive, [](const auto& notification, const ValidatorContext& context) {
		const auto& multisigCache = context.Cache.sub<cache::MultisigCache>();
		if (!multisigCache.contains(notification.Drive))
            return Failure_Service_Drive_Key_Is_Not_Multisig;

		const auto& driveCache = context.Cache.sub<cache::DriveCache>();
		if (driveCache.contains(notification.Drive))
            return Failure_Service_Drive_Alredy_Exists;

		if (notification.Duration.unwrap() == 0)
			return Failure_Service_Drive_Invalid_Duration;

		if (notification.Size == 0)
			return Failure_Service_Drive_Invalid_Size;

		if (notification.Replicas == 0)
			return Failure_Service_Drive_Invalid_Replicas;

		return ValidationResult::Success;
	});
}}
