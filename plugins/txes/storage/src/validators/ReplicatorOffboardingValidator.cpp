/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"

namespace catapult { namespace validators {

	using Notification = model::ReplicatorOffboardingNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(ReplicatorOffboarding, [](const Notification& notification, const ValidatorContext& context) {
	  	auto replicatorCache = context.Cache.sub<cache::ReplicatorCache>();
	  	auto replicatorIter = replicatorCache.find(notification.PublicKey);
		const auto& pReplicatorEntry = replicatorIter.tryGet();

	  	// Check if the signer is registered as replicator
		if (!pReplicatorEntry)
			return Failure_Storage_Replicator_Not_Registered;

	  	auto driveCache = context.Cache.sub<cache::BcDriveCache>();
	  	auto driveIter = driveCache.find(notification.DriveKey);
	  	const auto& pDriveEntry = driveIter.tryGet();

	  	// Check if respective drive exists
		if (!pDriveEntry)
		  	return Failure_Storage_Drive_Not_Found;

	  	// Check if the replicator is assigned to the drive
		if (!pDriveEntry->replicators().count(notification.PublicKey))
			return Failure_Storage_Drive_Not_Assigned_To_Replicator;

		// Check if the replicator hasn't applied for offboarding
		const auto& keys = pDriveEntry->offboardingReplicators();
		if (std::find(keys.begin(), keys.end(), notification.PublicKey) != keys.end())
		  	return Failure_Storage_Already_Applied_For_Offboarding;

		return ValidationResult::Success;
	});

}}
