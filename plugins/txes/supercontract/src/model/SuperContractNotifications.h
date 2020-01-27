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

}}
