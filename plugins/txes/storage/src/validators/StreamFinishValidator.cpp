/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"

namespace catapult { namespace validators {

	using Notification = model::StreamFinishNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(StreamFinish, [](const Notification& notification, const ValidatorContext& context) {
	  	auto driveCache = context.Cache.sub<cache::BcDriveCache>();
	  	auto driveIter = driveCache.find(notification.DriveKey);
		const auto& pDriveEntry = driveIter.tryGet();
		if (!pDriveEntry)
			return Failure_Storage_Drive_Not_Found;

		const auto& owner = pDriveEntry->owner();
		if (owner != notification.Owner) {
			return Failure_Storage_Is_Not_Owner;
		}

		if (pDriveEntry->activeDataModifications().empty()) {
			return Failure_Storage_No_Active_Data_Modifications;
		}

	  	const auto& activeDataModification = pDriveEntry->activeDataModifications().begin();
	  	if (activeDataModification->Id != notification.StreamId) {
			return Failure_Storage_Invalid_Stream_Id;
		}

		if (activeDataModification->ReadyForApproval) {
			return Failure_Storage_Stream_Already_Finished;
		}

		if (activeDataModification->ExpectedUploadSizeMegabytes < notification.ActualUploadSize) {
			return Failure_Storage_Expected_Upload_Size_Exceeded;
		}

		return ValidationResult::Success;
	});

}}
