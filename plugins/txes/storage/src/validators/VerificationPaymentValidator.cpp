/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/cache/BcDriveCache.h"

namespace catapult { namespace validators {

	using Notification = model::VerificationPaymentNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(VerificationPayment, [](const Notification& notification, const ValidatorContext& context) {
		const auto& driveCache = context.Cache.sub<cache::BcDriveCache>();
		const auto driveIter = driveCache.find(notification.DriveKey);
		const auto& pDriveEntry = driveIter.tryGet();

		// Check if drive exists
		if (!pDriveEntry)
			return Failure_Storage_Drive_Not_Found;

	  	// Check if transaction signer is the owner of the drive
	  	if (notification.Owner != pDriveEntry->owner())
		  	return Failure_Storage_Is_Not_Owner;

		return ValidationResult::Success;
	});

}}
