/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/model/Notifications.h"
#include "src/model/SuperContractTypes.h"
#include "catapult/utils/MemoryUtils.h"
#include <vector>

namespace catapult { namespace model {

	/// Defines a prepare drive notification type.
	DEFINE_NOTIFICATION_TYPE(All, SuperContract, Deploy_v1, 0x0001);

	/// Defines a prepare drive notification type.
	DEFINE_NOTIFICATION_TYPE(Validator, SuperContract, SuperContract_v1, 0x0002);

    /// Deploy super contract notification.
    template<VersionType version>
    struct DeployNotification;

    template<>
    struct DeployNotification<1> : public Notification {
    public:
        /// Matching notification type.
        static constexpr auto Notification_Type = SuperContract_Deploy_v1_Notification;

    public:
        /// Creates a notification around \a contract, \a owner, \a drive, \a file and \a version.
        explicit DeployNotification(
                const Key& contract,
                const Key& owner,
                const Key& drive,
                const Hash256& file,
                const BlockchainVersion& version)
                : Notification(Notification_Type, sizeof(DeployNotification<1>))
                , SuperContract(contract)
                , Owner(owner)
                , Drive(drive)
                , FileHash(file)
                , SuperContractVersion(version)
        {}

    public:
        /// Super contract key.
        const Key& SuperContract;

        /// Owner of super contract.
        const Key& Owner;

        /// Drive according to super contract.
        const Key& Drive;

        /// Hash of according file.
        const Hash256& FileHash;

        /// Version of super contract.
        BlockchainVersion SuperContractVersion;
    };

    /// Notification of a super contract.
    template<VersionType version>
    struct SuperContractNotification;

    template<>
    struct SuperContractNotification<1> : public Notification {
    public:
        /// Matching notification type.
        static constexpr auto Notification_Type = SuperContract_SuperContract_v1_Notification;

    public:
        explicit SuperContractNotification(const Key& contract)
                : Notification(Notification_Type, sizeof(SuperContractNotification<1>))
                , SuperContractKey(contract)
        {}

    public:
        /// Public key of the super contract multisig account.
        Key SuperContractKey;
    };

}}
