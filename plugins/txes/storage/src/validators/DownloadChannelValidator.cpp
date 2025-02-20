/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"

namespace catapult { namespace validators {

	using Notification = model::DownloadNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(DownloadChannel, ([](const Notification& notification, const ValidatorContext& context) {
	  	const auto& driveCache = context.Cache.sub<cache::BcDriveCache>();
	  	const auto driveIter = driveCache.find(notification.DriveKey);
	  	const auto& pDriveEntry = driveIter.tryGet();

	  	// Check if respective drive exists
	  	if (notification.DownloadSizeMegabytes == 0)
		  	return Failure_Storage_Download_Size_Insufficient;

	  	// Check if respective drive exists
	  	if (!pDriveEntry)
		  	return Failure_Storage_Drive_Not_Found;

	  	// Check if there are any replicators assigned to the drive
	  	if (pDriveEntry->replicators().empty())
		  	return Failure_Storage_No_Replicator;	// TODO: Change

		return ValidationResult::Success;
	}))

}}
