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
        const auto &driveCache = context.Cache.sub<cache::BcDriveCache>();
        const auto driveIter = driveCache.find(notification.DriveKey);
        const auto &pDriveEntry = driveIter.tryGet();

        // Check if respective drive exists
        if (!pDriveEntry)
                return Failure_Storage_Drive_Not_Found;

        auto& pendingVerification = pDriveEntry->verifications().back();

        // Check if provided verification trigger is equal to desired
        if (notification.VerificationTrigger != pendingVerification.VerificationTrigger)
            return Failure_Storage_Verification_Bad_Verification_Trigger;

        // Check if the count of Provers is right
        if (pendingVerification.Results.size() != notification.ProversCount)
            return Failure_Storage_Verification_Wrong_Namber_Of_Provers;

        // Check if all Provers were in the Confirmed state at the start of verification.
        for (auto i = 0; i < notification.ProversCount; ++i) {
            if (pendingVerification.Results.find(notification.ProversPtr[i]) == pendingVerification.Results.end())
                return Failure_Storage_Verification_Some_Provers_Are_Illegal;
        }

        // Check if the verification in the Pending state.
        if (pDriveEntry->verifications().back().State != state::VerificationState::Pending)
            return Failure_Storage_Verification_Not_In_Pending;

        return ValidationResult::Success;
    });
}}
