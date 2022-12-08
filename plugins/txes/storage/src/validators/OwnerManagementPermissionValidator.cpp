/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"

namespace catapult { namespace validators {

	using Notification = model::OwnerManagementProhibition<1>;

	DECLARE_STATEFUL_VALIDATOR(OwnerManagementProhibition, Notification)() {
		return MAKE_STATEFUL_VALIDATOR(OwnerManagementProhibition, [](const Notification& notification, const ValidatorContext& context) {
			const auto& driveCache = context.Cache.sub<cache::BcDriveCache>();
            auto driveIter = driveCache.find(notification.DriveKey);
            auto* pDriveEntry = driveIter.tryGet();

            if (!pDriveEntry) {
                return Failure_Storage_Drive_Not_Found;
            }

			if (pDriveEntry->ownerManagement() != state::OwnerManagement::TEMPORARY_FORBIDDEN) {
				return Failure_Storage_Owner_Management_Can_Not_Be_Allowed;
			}

			return ValidationResult::Success;
		})
	}
}}
