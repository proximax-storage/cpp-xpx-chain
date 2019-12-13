/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/cache/DriveCache.h"
#include "catapult/validators/ValidatorContext.h"

namespace catapult { namespace validators {

	using Notification = model::PrepareDriveNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(PrepareDrivePermission, [](const Notification& notification, const ValidatorContext& context) {
		const auto& driveCache = context.Cache.sub<cache::DriveCache>();
		if (driveCache.contains(notification.DriveKey))
			return Failure_Service_Drive_Already_Exists;

		return ValidationResult::Success;
	});
}}
