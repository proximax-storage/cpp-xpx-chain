/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/cache/DriveCache.h"

namespace catapult { namespace validators {

	using Notification = model::StartDriveVerificationNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(StartDriveVerification, [](const Notification &notification, const ValidatorContext &context) {
		const auto& driveCache = context.Cache.sub<cache::DriveCache>();
		auto driveIter = driveCache.find(notification.DriveKey);
		const auto &driveEntry = driveIter.get();
		if (driveEntry.state() != state::DriveState::InProgress)
			return Failure_Service_Drive_Is_Not_In_Progress;

		bool verificationStarted;
		bool verificationActive;
		VerificationStatus(driveEntry, context, verificationStarted, verificationActive);
		if (verificationActive)
			return Failure_Service_Verification_Already_In_Progress;
		if (verificationStarted)
			return Failure_Service_Verification_Has_Not_Timed_Out;

		if (!driveEntry.isOwner(notification.Initiator) && !driveEntry.replicators().count(notification.Initiator))
			return Failure_Service_Operation_Is_Not_Permitted;

		return ValidationResult::Success;
	});
}}
