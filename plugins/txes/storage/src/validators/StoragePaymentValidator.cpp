/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/cache/BcDriveCache.h"

namespace catapult { namespace validators {

	using Notification = model::StoragePaymentNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(StoragePayment, [](const model::StoragePaymentNotification<1>& notification, const ValidatorContext& context) {
		const auto driveCache = context.Cache.sub<cache::BcDriveCache>();
	  	const auto driveIter = driveCache.find(notification.DriveKey);
	  	const auto& pDriveEntry = driveIter.tryGet();

	  	// Check if drive exists
		if (!pDriveEntry)
		  	return Failure_Storage_Drive_Not_Found;

		return ValidationResult::Success;
	});

}}
