/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"

namespace catapult { namespace validators {

    using Notification = model::EndDriveVerificationNotification<1>;

    DEFINE_STATEFUL_VALIDATOR(EndDriveVerification, [](const Notification& notification, const ValidatorContext& context) {
        const auto& driveCache = context.Cache.sub<cache::BcDriveCache>();
		if (!driveCache.contains(notification.DriveKey))
            return Failure_Storage_Drive_Not_Found;

        const auto driveIter = driveCache.find(notification.DriveKey);
        const auto& driveEntry = driveIter.get();

		if (!driveEntry.verification() || driveEntry.verification()->expired(context.BlockTime))
            return Failure_Storage_Verification_Not_In_Progress;

		const auto& verification = *driveEntry.verification();
		if (verification.VerificationTrigger != notification.VerificationTrigger)
            return Failure_Storage_Bad_Verification_Trigger;

		const auto& shards = verification.Shards;
	  	if (notification.ShardId >= shards.size())
		  	return Failure_Storage_Verification_Invalid_Shard_Id;

	  	const auto& shard = shards[notification.ShardId];
	  	if (shard.empty())
		  	return Failure_Storage_Transaction_Already_Approved;

		if (shard.size() != notification.KeyCount)
			return Failure_Storage_Verification_Invalid_Prover_Count;

		if ( notification.JudgingKeyCount < (shard.size() * 2) / 3 + 1 ) {
			return Failure_Storage_Signature_Count_Insufficient;
		}

		auto pPublicKey = notification.PublicKeysPtr;
		for (auto i = 0u; i < notification.JudgingKeyCount; ++i, ++pPublicKey) {
			if (!shard.count(*pPublicKey))
				return Failure_Storage_Opinion_Invalid_Key;
		}

        return ValidationResult::Success;
    });
}}
