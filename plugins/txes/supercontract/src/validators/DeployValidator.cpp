/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/cache/DriveCache.h"
#include "src/cache/SuperContractCache.h"

namespace catapult { namespace validators {

	using Notification = model::DeployNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(Deploy, [](const Notification& notification, const ValidatorContext& context) {
		const auto& superContractCache = context.Cache.sub<cache::SuperContractCache>();
		if (superContractCache.contains(notification.SuperContract))
			return Failure_SuperContract_Super_Contract_Already_Exists;

		const auto& driveCache = context.Cache.sub<cache::DriveCache>();
		auto driveIter = driveCache.find(notification.Drive);
		const state::DriveEntry& driveEntry = driveIter.get();

		if (!driveEntry.isOwner(notification.Owner))
			return Failure_SuperContract_Operation_Is_Not_Permitted;

		if (!driveEntry.files().count(notification.FileHash))
			return Failure_SuperContract_File_Does_Not_Exist;

		return ValidationResult::Success;
	});
}}
