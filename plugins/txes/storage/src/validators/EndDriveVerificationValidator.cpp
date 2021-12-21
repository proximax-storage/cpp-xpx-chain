/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <catapult/crypto/Signer.h>
#include "Validators.h"

namespace catapult { namespace validators {

    using Notification = model::EndDriveVerificationNotification<1>;

    DEFINE_STATEFUL_VALIDATOR(EndDriveVerification, [](const Notification& notification, const ValidatorContext& context) {
        const auto& driveCache = context.Cache.sub<cache::BcDriveCache>();
        const auto driveIter = driveCache.find(notification.DriveKey);
        const auto& pDriveEntry = driveIter.tryGet();

        // Check if respective drive exists
        if (!pDriveEntry)
            return Failure_Storage_Drive_Not_Found;

        // Check if provided verification trigger exist and if it has Pending state
        auto pendingVerification = std::find_if(
                pDriveEntry->verifications().begin(),
                pDriveEntry->verifications().end(),
                [&notification](const state::Verification& v) {
                    return v.VerificationTrigger == notification.VerificationTrigger &&
                           v.State == state::VerificationState::Pending;
                }
        );

        if (pendingVerification == pDriveEntry->verifications().end())
            return Failure_Storage_Verification_Bad_Verification_Trigger;

        // Check if the count of Provers is right
        if (pendingVerification->Results.size() != notification.ProversCount)
            return Failure_Storage_Verification_Wrong_Number_Of_Provers;

        // Check if all Provers were in the Confirmed state at the start of verification.
        for (auto i = 0; i < notification.ProversCount; ++i) {
            if (pendingVerification->Results.find(notification.ProversPtr[i]) == pendingVerification->Results.end())
                return Failure_Storage_Verification_Some_Provers_Are_Illegal;
        }

		// TODO: Check that ProversCount >= VerificationOpinionsCount

		// Check if all indices in VerificationOpinions are valid
		// TODO: Check for reoccurring and unused indices
	  	std::vector<std::vector<bool>> presentOpinions;
	  	presentOpinions.resize(notification.VerificationOpinionsCount);
	  	auto pVerificationOpinion = notification.VerificationOpinionsPtr;
	  	for (auto i = 0u; i < notification.VerificationOpinionsCount; ++i, ++pVerificationOpinion) {
		  	const auto& verifierIndex = pVerificationOpinion->Verifier;
			if (verifierIndex > notification.VerificationOpinionsCount)
				return Failure_Storage_Opinion_Invalid_Index;
			presentOpinions.at(verifierIndex).resize(notification.ProversCount, false);
		  	for (const auto& result : pVerificationOpinion->Results) {
				if (result.first > notification.ProversCount)
					return Failure_Storage_Opinion_Invalid_Index;
				presentOpinions.at(verifierIndex).at(result.first) = true;
			}
	  	}

	  	// Check if the order of keys in Provers is valid (all judging keys should go before overlapping and judged ones)
	  	auto switchesCount = 0u;	// Number of switches between series of empty and non-empty columns in presentOpinions
		bool previousColumnIsEmpty = true;
	  	for (auto j = 0u; j < notification.ProversCount; ++j) {
		  	bool currentColumnIsEmpty = true;
		  	for (auto i = 0u; i < notification.VerificationOpinionsCount; ++i)
			  	if (presentOpinions.at(i).at(j)) {
					currentColumnIsEmpty = false;
				  	break;
			  	}
		  	if (currentColumnIsEmpty != previousColumnIsEmpty) {
				++switchesCount;
				if (switchesCount > 1)
					return Failure_Storage_Opinion_Invalid_Key_Order;
				previousColumnIsEmpty = currentColumnIsEmpty;
			}
	  	}

        return ValidationResult::Success;
    });
}}
