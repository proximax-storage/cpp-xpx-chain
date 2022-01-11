/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
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
			const uint16_t shardId,
			const uint16_t keyCount,
			const uint16_t judgingKeyCount,
			const Key* pPublicKeys,
			const Signature* pSignatures,
			const uint8_t* pOpinions)
                : Notification(Notification_Type, sizeof(EndDriveVerificationNotification<1>))
                , DriveKey(driveKey)
                , VerificationTrigger(verificationTrigger)
                , ShardId(shardId)
                , KeyCount(keyCount)
                , JudgingKeyCount(judgingKeyCount)
                , PublicKeysPtr(pPublicKeys)
                , SignaturesPtr(pSignatures)
                , OpinionsPtr(pOpinions)
		{}

    public:
        /// Key of the drive.
        Key DriveKey;

        /// The hash of block that initiated the Verification.
        Hash256 VerificationTrigger;

        /// Shard identifier.
        uint16_t ShardId;

        /// Number of replicators.
        uint16_t KeyCount;

		/// Number of replicators that provided their opinions.
		uint8_t JudgingKeyCount;

        /// Array of the replicator keys.
        const Key* PublicKeysPtr;

        /// Array or signatures.
        const Signature* SignaturesPtr;

        /// Array or signatures.
        const uint8_t* OpinionsPtr;
    };
}}
