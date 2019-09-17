/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/cache/DriveCache.h"
#include "catapult/validators/ValidatorContext.h"

namespace catapult { namespace validators {

	using Notification = model::FileDepositNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(FileDeposit, [](const auto& notification, const ValidatorContext& context) {
		const auto& driveCache = context.Cache.sub<cache::DriveCache>();
		auto driveEntry = driveCache.find(notification.Drive).get();
		auto& replicatorDepositMap = driveEntry.replicators()[notification.Replicator];
		if (replicatorDepositMap.count(notification.FileHash) == 0)
            return Failure_Service_File_Deposit_Already_Returned;

		auto replicatorDeposit = replicatorDepositMap[notification.FileHash];
		if (replicatorDeposit.Amount > notification.Deposit.Amount)
            return Failure_Service_Returned_File_Deposit_Invalid;

		return ValidationResult::Success;
	});
}}
