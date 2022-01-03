/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/cache/BcDriveCache.h"

namespace catapult { namespace validators {

	using Notification = model::DataModificationApprovalUploadWorkNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(DataModificationApprovalUploadWork, [](const Notification& notification, const ValidatorContext& context) {
	  	const auto& driveCache = context.Cache.sub<cache::BcDriveCache>();
	  	const auto driveIter = driveCache.find(notification.DriveKey);
	 	const auto& pDriveEntry = driveIter.tryGet();

	  	// Check if respective drive exists
	  	if (!pDriveEntry)
		  	return Failure_Storage_Drive_Not_Found;

	  	const auto& accountStateCache = context.Cache.sub<cache::AccountStateCache>();
	  	const auto senderStateIter = accountStateCache.find(notification.DriveKey);
	  	const auto& pSenderState = senderStateIter.tryGet();

	  	// Check if account state of the drive exists
	  	if (!pSenderState)
		  	return Failure_Storage_Sender_State_Not_Found;

		// Check if
		// - all uploaders' account states exist
	  	// - all replicators' keys are present in drive's cumulativeUploadSizes
		const auto totalJudgedKeysCount = notification.OverlappingKeysCount + notification.JudgedKeysCount;
	  	const auto& driveOwnerPublicKey = pDriveEntry->owner();
		auto pKey = &notification.PublicKeysPtr[notification.JudgingKeysCount];
		for (auto i = 0; i < totalJudgedKeysCount; ++i, ++pKey) {
			const auto recipientStateIter = accountStateCache.find(*pKey);
			const auto& pRecipientState = recipientStateIter.tryGet();
			if (!pRecipientState)
				return Failure_Storage_Recipient_State_Not_Found;
		}

	  	// TODO: Check if there are enough mosaics for the transfer?

		return ValidationResult::Success;
	});
}}
