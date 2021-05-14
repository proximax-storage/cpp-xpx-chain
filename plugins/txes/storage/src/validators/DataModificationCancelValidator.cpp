/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/cache/DriveCache.h"

namespace catapult { namespace validators {

	using Notification = model::DataModificationCancelNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(DataModificationCancel,[](const model::DataModificationCancelNotification<1>& notification, const ValidatorContext& context) {
		const auto& driveCache = context.Cache.sub<cache::DriveCache>();
		const auto driveIter = driveCache.find(notification.DriveKey);
		const auto& driveEntry = driveIter.get();
		const auto& activeDataModifications = driveEntry.activeDataModifications();
		
		if (notification.Owner != driveEntry.owner())
		  return Failure_Storage_Is_Not_Owner;

		if (activeDataModifications.empty())
			return Failure_Storage_No_Active_Data_Modifications;

		if (activeDataModifications.front() == notification.ModificationTrx)
			return Failure_Storage_Data_Modification_Is_Active;

		auto dataModificationIter = std::find(
				++activeDataModifications.begin(),
				activeDataModifications.end(),
				notification.ModificationTrx);

		if (dataModificationIter == activeDataModifications.end())
			return Failure_Storage_Data_Modification_Not_Found;

		return ValidationResult::Success;
	})
}}
