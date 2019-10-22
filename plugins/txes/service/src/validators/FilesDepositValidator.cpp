/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/cache/DriveCache.h"
#include "catapult/validators/ValidatorContext.h"

namespace catapult { namespace validators {

	using Notification = model::FilesDepositNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(FileDepositReturn, [](const Notification& notification, const ValidatorContext& context) {
		const auto& driveCache = context.Cache.sub<cache::DriveCache>();
		if (!driveCache.contains(notification.DriveKey))
			return Failure_Service_Drive_It_Not_Exist;

		const auto& driveEntry = driveCache.find(notification.DriveKey).get();

		if (driveEntry.replicators().count(notification.Replicator))
            return Failure_Service_Drive_Replicator_Not_Registered;

		const auto& filesWithoutDeposit = driveEntry.replicators().find(notification.Replicator)->second.FilesWithoutDeposit;

		auto filesPtr = notification.FilesPtr;
		for (auto i = 0u; i < notification.FilesCount; ++i, ++filesPtr)
			if (!filesWithoutDeposit.count(filesPtr->FileHash))
				return Failure_Service_File_Is_Not_Exist;

		return ValidationResult::Success;
	});
}}
