/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/cache/BcDriveCache.h"

namespace catapult { namespace validators {

	using Notification = model::DataModificationApprovalRefundNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(DataModificationApprovalRefund, [](const Notification& notification, const ValidatorContext& context) {
	  	const auto& driveCache = context.Cache.sub<cache::BcDriveCache>();
	  	const auto driveIter = driveCache.find(notification.DriveKey);
	 	const auto& pDriveEntry = driveIter.tryGet();

	  	// Check if respective drive exists
	  	if (!pDriveEntry)
		  	return Failure_Storage_Drive_Not_Found;

	  	// Check if there are any active data modifications
		const auto& activeDataModifications = pDriveEntry->activeDataModifications();
	  	if (activeDataModifications.empty())
			return Failure_Storage_No_Active_Data_Modifications;

	  	// Check if respective data modification is the first (oldest) element in activeDataModifications
	  	if (activeDataModifications.begin()->Id != notification.DataModificationId)
		  	return Failure_Storage_Invalid_Data_Modification_Id;

	  	const auto usedSizeDifference = pDriveEntry->activeDataModifications().begin()->ActualUploadSize
				+ pDriveEntry->usedSize()
				- (notification.UsedDriveSize - notification.MetaFilesSize);
		if (usedSizeDifference < 0)
			return Failure_Storage_Invalid_Used_Size;

		// TODO: Check if UsedDriveSize - MetaFilesSize > 0?

		// TODO: Check if there are enough mosaics for the transfer?

		return ValidationResult::Success;
	});
}}
