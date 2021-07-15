/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/cache/BcDriveCache.h"

namespace catapult { namespace validators {

	using Notification = model::DriveClosureNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(DriveClosure, [](const Notification& notification, const ValidatorContext& context) {
        const auto& driveCache = context.Cache.sub<cache::BcDriveCache>();
        
        // Check if the drive exists
        if (driveCache.contains(notification.DriveKey))
            return ValidationResult::Success;
		
        return Failure_Storage_Drive_Not_Found;
		
	})
}}
