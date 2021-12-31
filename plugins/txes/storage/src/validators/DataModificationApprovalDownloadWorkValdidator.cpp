/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/cache/BcDriveCache.h"

namespace catapult { namespace validators {

	using Notification = model::DataModificationApprovalDownloadWorkNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(DataModificationApprovalDownloadWork, [](const Notification& notification, const ValidatorContext& context) {
	  	const auto& driveCache = context.Cache.sub<cache::BcDriveCache>();
	  	const auto driveIter = driveCache.find(notification.DriveKey);
	 	const auto& pDriveEntry = driveIter.tryGet();

	  	// Check if respective drive exists
	  	if (!pDriveEntry)
		  	return Failure_Storage_Drive_Not_Found;

	  	const auto& replicatorCache = context.Cache.sub<cache::ReplicatorCache>();
	  	auto pKey = notification.PublicKeysPtr;
		for (auto i = 0; i < notification.PublicKeysCount; ++i, ++pKey) {
			// Check if judging replicator exists
			const auto replicatorIter = replicatorCache.find(*pKey);
			const auto& pReplicatorEntry = replicatorIter.tryGet();
			if (!pReplicatorEntry)
				return Failure_Storage_Replicator_Not_Found;

			// Check if the replicator has respective drive info with given drive key
			if (!pReplicatorEntry->drives().count(notification.DriveKey))
				return Failure_Storage_Drive_Info_Not_Found;

			// Check if the replicator key is present in drive's confirmedUsedSizes
			// TODO: Double-check if needed
			if (!pDriveEntry->confirmedUsedSizes().count(*pKey))
				return Failure_Storage_Replicator_Not_Found;
		}

	  	// TODO: Check if there are enough mosaics for the transfer?

		return ValidationResult::Success;
	});
}}
