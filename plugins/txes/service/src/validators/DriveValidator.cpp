/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/cache/DriveCache.h"
#include "catapult/validators/ValidatorContext.h"
#include "src/model/ServiceEntityType.h"
#include <unordered_set>

namespace catapult { namespace validators {

	using Notification = model::DriveNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(Drive, [](const Notification& notification, const ValidatorContext& context) {
		const auto& driveCache = context.Cache.sub<cache::DriveCache>();
		if (!driveCache.contains(notification.DriveKey))
			return Failure_Service_Drive_Does_Not_Exist;

		static std::unordered_set<model::EntityType> blockedTransactionAfterFinish({
		    model::Entity_Type_DriveFileSystem,
		    model::Entity_Type_FilesDeposit,
		    model::Entity_Type_JoinToDrive,
		    model::Entity_Type_EndDrive,
		    model::Entity_Type_Start_Drive_Verification,
		    model::Entity_Type_End_Drive_Verification,
		});

		if (blockedTransactionAfterFinish.count(notification.TransactionType)) {
			auto driveIter = driveCache.find(notification.DriveKey);
			const auto& driveEntry = driveIter.get();

			if (driveEntry.state() >= state::DriveState::Finished)
				return Failure_Service_Drive_Has_Ended;
		}

		return ValidationResult::Success;
	});
}}
