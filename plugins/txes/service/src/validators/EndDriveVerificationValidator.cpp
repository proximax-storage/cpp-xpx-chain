/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/cache/DriveCache.h"
#include "src/config/ServiceConfiguration.h"

namespace catapult { namespace validators {

	using Notification = model::EndDriveVerificationNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(EndDriveVerification, [](const Notification &notification, const StatefulValidatorContext&context) {
		const auto& driveCache = context.Cache.sub<cache::DriveCache>();
		auto driveIter = driveCache.find(notification.DriveKey);
		const state::DriveEntry& driveEntry = driveIter.get();
		bool verificationStarted;
		bool verificationActive;
		VerificationStatus(driveEntry, context, verificationStarted, verificationActive);
		if (!verificationStarted)
				return Failure_Service_Verification_Has_Not_Started;
		if (!verificationActive)
				return Failure_Service_Verification_Is_Not_Active;

		std::set<Key> keys;
		auto pFailedReplicators = notification.FailedReplicatorsPtr;
		for (auto i = 0u; i < notification.FailureCount; ++i) {
			keys.insert(pFailedReplicators[i]);
			if (!driveEntry.replicators().count(pFailedReplicators[i]))
				return Failure_Service_Drive_Replicator_Not_Registered;
		}

		if (keys.size() != notification.FailureCount)
			return Failure_Service_Participant_Redundant;

		return ValidationResult::Success;
	});
}}
