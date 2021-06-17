/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/cache/BcDriveCache.h"

namespace catapult { namespace validators {

	using Notification = model::DataModificationSingleApprovalNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(DataModificationSingleApproval, [](const Notification& notification, const ValidatorContext& context) {
	  	const auto& driveCache = context.Cache.sub<cache::BcDriveCache>();
		const auto driveIter = driveCache.find(notification.DriveKey);
		const auto& pDriveEntry = driveIter.tryGet();

		// Check if respective drive exists
		if (!pDriveEntry)
			return Failure_Storage_Drive_Not_Found;

		// Check if there are any approved data modifications
		const auto& completedDataModifications = pDriveEntry->completedDataModifications();
		const auto& lastApprovedDataModification = std::find_if(
				completedDataModifications.rbegin(),
				completedDataModifications.rend(),
				[](const state::CompletedDataModification& dataModification) {return dataModification.State == state::DataModificationState::Succeeded;});
	  	if (lastApprovedDataModification == completedDataModifications.rend())
		  	return Failure_Storage_No_Approved_Data_Modifications;

	  	// Check if respective data modification is the last (newest) among approved data modifications
		if (lastApprovedDataModification->Id != notification.DataModificationId)
			return Failure_Storage_Invalid_Data_Modification_Id;

		std::set<Key> uploaderKeys;
	  	uint8_t cumulativePercentage = 0;
	  	const auto& replicatorCache = context.Cache.sub<cache::ReplicatorCache>();
	  	auto pKey = notification.UploaderKeysPtr;
	  	auto pPercent = notification.UploadOpinionPtr;
	  	for (auto i = 0u; i < notification.UploadOpinionPairCount; ++i, ++pKey, ++pPercent) {

			// Check if the key appears in upload opinion exactly once
			if (uploaderKeys.count(*pKey) != 0)
				return Failure_Storage_Opinion_Reocurring_Keys;

			uploaderKeys.insert(*pKey);
			cumulativePercentage += *pPercent;

			// Check if the key is a key of the Drive Owner
			if (pDriveEntry->owner() == *pKey)
				continue;

			// Check if the key is a key of one of the Drive's current Replicators
			const auto replicatorIter = replicatorCache.find(*pKey);
			const auto& pReplicatorEntry = replicatorIter.tryGet();
			if (pReplicatorEntry) {
				const auto& drives = pReplicatorEntry->drives();
				if (std::find(drives.begin(), drives.end(), notification.DriveKey) != drives.end())
					continue;
			}

			return Failure_Storage_Opinion_Invalid_Key;
		}

		// Check if the percents in the upload opinion sum up to 100
		if (cumulativePercentage != 100)
			return Failure_Storage_Opinion_Incorrect_Percentage;

		return ValidationResult::Success;
	});

}}
