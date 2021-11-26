/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once

#include "src/model/StorageTypes.h"
#include "catapult/model/Notifications.h"
#include "catapult/model/StorageNotifications.h"

namespace catapult { namespace model {
    /// Notification of end drive verification.
    template<VersionType version>
    struct EndDriveVerificationNotification;

    template<>
    struct EndDriveVerificationNotification<1> : public Notification {
    public:
        /// Matching notification type.
        static constexpr auto Notification_Type = Storage_End_Drive_Verification_v1_Notification;

    public:
        explicit EndDriveVerificationNotification(
                const Key& driveKey,
                const Hash256& verificationTrigger,
                const uint16_t proversCount,
                const Key* proversPtr,
                const uint16_t verificationOpinionsCount,
                const VerificationOpinion* verificationOpinionsPtr)
                : Notification(Notification_Type, sizeof(EndDriveVerificationNotification<1>))
                , DriveKey(driveKey)
                , VerificationTrigger(verificationTrigger)
                , ProversCount(proversCount)
                , ProversPtr(proversPtr)
                , VerificationOpinionsCount(verificationOpinionsCount)
                , VerificationOpinionsPtr(verificationOpinionsPtr) {}

    public:
        /// Key of the drive.
        Key DriveKey;

        /// The hash of block that initiated the Verification.
        Hash256 VerificationTrigger;

        /// Number of Provers.
        uint16_t ProversCount;

        /// List of the Provers keys.
        const Key* ProversPtr;

        /// Number of verification opinions in the payload.
        uint16_t VerificationOpinionsCount;

        /// Opinion about verification status for each Prover. Success or Failure.
        const VerificationOpinion* VerificationOpinionsPtr;
    };

    /// Notification of start drive verification.
    template<VersionType version>
    struct StartDriveVerificationNotification;

    template<>
    struct StartDriveVerificationNotification<1> : public Notification {
    public:
        /// Matching notification type.
        static constexpr auto Notification_Type = Storage_Start_Drive_Verification_v1_Notification;

    public:
        explicit StartDriveVerificationNotification(const Key& driveKey, const Hash256& blockHash)
                : Notification(Notification_Type, sizeof(StartDriveVerificationNotification<1>))
                , DriveKey(driveKey)
                , BlockHash(blockHash){}

    public:
        /// Key of the drive.
        Key DriveKey;

        /// Current block hash.
        Hash256 BlockHash;

        /// Current block generation time.
        utils::TimeSpan BlockGenerationTime;
    };
}}
