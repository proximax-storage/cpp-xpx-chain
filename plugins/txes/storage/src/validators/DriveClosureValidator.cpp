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
        auto driveIter = driveCache.find(notification.DriveKey);
        const auto& pDriveEntry = driveIter.tryGet();
        if (!pDriveEntry)
        	return Failure_Storage_Drive_Not_Found;

		// Check of the signer is the owner
        const auto& owner = pDriveEntry->owner();
        if (owner != notification.DriveOwner) {
        	return Failure_Storage_Is_Not_Owner;
        }

        return ValidationResult::Success;
		
	})
}}
