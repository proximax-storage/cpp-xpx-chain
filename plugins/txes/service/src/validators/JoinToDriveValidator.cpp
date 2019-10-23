/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/cache/DriveCache.h"
#include "catapult/validators/ValidatorContext.h"

namespace catapult { namespace validators {

	using Notification = model::JoinToDriveNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(JoinToDrive, [](const Notification& notification, const ValidatorContext& context) {
		const auto& driveCache = context.Cache.sub<cache::DriveCache>();
		if (!driveCache.contains(notification.DriveKey))
			return Failure_Service_Drive_Does_Not_Exist;

		const auto& drive = driveCache.find(notification.DriveKey).get();
		if (drive.hasReplicator(notification.Replicator))
			return Failure_Service_Replicator_Already_Connected_To_Drive;

		return ValidationResult::Success;
	});
}}
