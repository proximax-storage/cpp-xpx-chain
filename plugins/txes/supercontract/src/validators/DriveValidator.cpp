/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/cache/DriveCache.h"
#include "src/model/SuperContractEntityType.h"

namespace catapult { namespace validators {

	using Notification = model::DriveNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(Drive, [](const Notification& notification, const ValidatorContext& context) {
		const auto& driveCache = context.Cache.sub<cache::DriveCache>();

		static std::unordered_set<model::EntityType> blockedTransactionAfterFinish({
		    model::Entity_Type_Deploy,
		    model::Entity_Type_StartExecute,
		    model::Entity_Type_EndExecute,
		});

		if (blockedTransactionAfterFinish.count(notification.TransactionType)) {
			auto driveIter = driveCache.find(notification.DriveKey);
			const auto& driveEntry = driveIter.get();

			if (driveEntry.state() >= state::DriveState::Finished)
				return Failure_SuperContract_Drive_Has_Ended;
		}

		return ValidationResult::Success;
	});
}}
