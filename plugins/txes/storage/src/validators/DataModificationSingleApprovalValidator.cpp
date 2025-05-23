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
		const auto& replicatorCache = context.Cache.sub<cache::ReplicatorCache>();
		const auto replicatorIter = replicatorCache.find(notification.PublicKey);
		const auto& pReplicatorEntry = replicatorIter.tryGet();

		// Check if respective replicator exists
		if (!pReplicatorEntry)
			return Failure_Storage_Replicator_Not_Found;

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
				[](const state::CompletedDataModification& dataModification) {return dataModification.ApprovalState == state::DataModificationApprovalState::Approved;});
	  	if (lastApprovedDataModification == completedDataModifications.rend())
		  	return Failure_Storage_No_Approved_Data_Modifications;

	  	// Check if respective data modification is the last (newest) among approved data modifications
		if (lastApprovedDataModification->Id != notification.DataModificationId)
			return Failure_Storage_Invalid_Data_Modification_Id;

		// Check if respective data modification hasn't already been approved by the replicator
	  	const auto& driveInfo = pReplicatorEntry->drives().at(notification.DriveKey);
	  	if (driveInfo.LastApprovedDataModificationId == notification.DataModificationId)
		  	return Failure_Storage_Transaction_Already_Approved;

		// This is a workaround to prevent single approvals after drive is deployed
		// Note that rarely a situation can occur when a replicator does not receive payment
		// for downloading drive data because contract is deployed earlier
		if (pDriveEntry->ownerManagement() == state::OwnerManagement::PERMANENTLY_FORBIDDEN) {
			return Failure_Storage_Owner_Management_Is_Forbidden;
		}

		return ValidationResult::Success;
	});

}}
