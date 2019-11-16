/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/cache/DriveCache.h"
#include "catapult/validators/ValidatorContext.h"

namespace catapult { namespace validators {

	using Notification = model::RewardNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(Reward, [](const Notification& notification, const ValidatorContext& context) {
		const auto& driveCache = context.Cache.sub<cache::DriveCache>();
		auto driveIter = driveCache.find(notification.DriveKey);
		const auto& driveEntry = driveIter.get();

		if (!driveEntry.hasFile(notification.DeletedFile->FileHash))
		    return Failure_Service_File_Is_Not_Exist;

		if (driveEntry.files().at(notification.DeletedFile->FileHash).Deposit.unwrap() == 0)
			return Failure_Service_File_Deposit_Is_Zero;

		if (notification.DeletedFile->InfosCount() == 0)
			return Failure_Service_Zero_Infos;

		auto pInfo = notification.DeletedFile->InfosPtr();
		for (auto i = 0u; i < notification.DeletedFile->InfosCount(); ++i, ++pInfo) {
		    if (!driveEntry.hasReplicator(pInfo->Participant) && driveEntry.owner() != pInfo->Participant)
		        return Failure_Service_Participant_It_Not_Part_Of_Drive;
		}

		return ValidationResult::Success;
	});
}}
