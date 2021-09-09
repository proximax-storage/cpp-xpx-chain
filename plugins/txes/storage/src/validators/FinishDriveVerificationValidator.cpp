/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <catapult/crypto/Signer.h>
#include "Validators.h"

namespace catapult { namespace validators {

    using Notification = model::FinishDriveVerificationNotification<1>;

    Hash256& calculateSharedFieldsHash(const Key& driveKey, const Hash256& verificationTrigger) {
        auto sharedDataSize = driveKey.size() + verificationTrigger.size();
        auto *const pSharedDataBegin = new uint8_t[sharedDataSize];
        auto *pSharedData = pSharedDataBegin;

        std::copy(driveKey.begin(), driveKey.end(), pSharedData);
        pSharedData += sizeof(driveKey.size());
        std::copy(verificationTrigger.begin(), verificationTrigger.end(), pSharedData);

        Hash256 verificationHash;
        crypto::Sha3_256(utils::RawBuffer(pSharedDataBegin, sharedDataSize), verificationHash);

        return verificationHash;
    }

    DEFINE_STATEFUL_VALIDATOR(FinishDriveVerification, [](const Notification& notification, const ValidatorContext& context) {
        const auto &driveCache = context.Cache.sub<cache::BcDriveCache>();
        const auto driveIter = driveCache.find(notification.DriveKey);
        const auto &pDriveEntry = driveIter.tryGet();

        // Check if respective drive exists
        if (!pDriveEntry)
                return Failure_Storage_Drive_Not_Found;

        // Check if all signer/provers are in the Confirmed state.
        for (auto i = 0; i < notification.VerifiersOpinionsCount; ++i) {
            if (pDriveEntry->confirmedStates().find(notification.ProversPtr[i])->second != pDriveEntry->rootHash())
                return Failure_Storage_Verification_Not_All_Signer_In_Confirmed_State;
        }

        return ValidationResult::Success;
    });
}}
