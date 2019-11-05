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
		auto driveIter = driveCache.find(notification.DriveKey);
		const auto& driveEntry = driveIter.get();
		if (driveEntry.hasReplicator(notification.Replicator))
			return Failure_Service_Replicator_Already_Connected_To_Drive;

		return ValidationResult::Success;
	});
}}
