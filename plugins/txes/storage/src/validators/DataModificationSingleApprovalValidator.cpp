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

	  	// Check if the transaction signer is a replicator of the drive
	  	const auto& replicators = pDriveEntry->replicators();
		if (!replicators.count(notification.PublicKey))
		  	return Failure_Storage_Invalid_Transaction_Signer;

	  	// Validating that each provided public key
	  	// - is unique
	  	// - is not a signer key
		// - is either a replicator key or a drive owner key
	  	std::set<Key> providedKeys;
	  	auto pKey = notification.PublicKeysPtr;
	  	const auto& driveOwner = pDriveEntry->owner();
	  	for (auto i = 0; i < notification.PublicKeysCount; ++i, ++pKey) {
		  	if (providedKeys.count(*pKey))
			  	return Failure_Storage_Opinion_Duplicated_Keys;
			providedKeys.insert(*pKey);
			if (*pKey == notification.PublicKey)
				return Failure_Storage_Opinion_Provided_On_Self;
			if (!replicators.count(*pKey) && *pKey != driveOwner)
				return Failure_Storage_Opinion_Invalid_Key;
	  	}

	  	// Check if the drive has any approved data modifications; if it has, find the last (newest) such modification
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

		return ValidationResult::Success;
	});

}}
