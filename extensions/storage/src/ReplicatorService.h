/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once

#include "StorageConfiguration.h"
#include "catapult/types.h"
#include "catapult/crypto/KeyPair.h"
#include "catapult/extensions/ServiceRegistrar.h"

namespace catapult { namespace model { class Transaction; } }

namespace catapult { namespace storage {

    class ReplicatorService {
    public:
        ReplicatorService(crypto::KeyPair&& keyPair, StorageConfiguration&& storageConfig);

    public:
        void start();
        void stop();

        void setServiceState(extensions::ServiceState* pServiceState) {
            m_pServiceState = pServiceState;
        }

        const Key& replicatorKey() const;

        bool isReplicatorRegistered(const Key& key);

        void addDriveModification(const Key& driveKey, const Hash256& downloadDataCdi, const Hash256& modificationId, const Key& owner, uint64_t dataSize);
        void removeDriveModification(const Key& driveKey, const Hash256& dataModificationId);

        void addDownloadChannel(const Hash256& channelId);
        void increaseDownloadChannelSize(const Hash256& channelId, size_t downloadSize);
        void closeDownloadChannel(const Hash256& channelId);

        void addDrive(const Key& driveKey, uint64_t driveSize);
        bool isAssignedToDrive(const Key& driveKey);
        void closeDrive(const Key& driveKey, const Hash256& transactionHash);

        void maybeCancelVerifications();

    public:
        void dataModificationApprovalPublished(const Key& driveKey, const Hash256& modificationId, const Hash256& rootHash, std::vector<Key>& replicators);
        void dataModificationSingleApprovalPublished(const Key& driveKey, const Hash256& modificationId);

    public:
        void notifyTransactionStatus(const Hash256& hash, uint32_t status);

    private:
        class Impl;

        std::shared_ptr<Impl> m_pImpl;

        crypto::KeyPair m_keyPair;
        StorageConfiguration m_storageConfig;
        extensions::ServiceState* m_pServiceState;
    };

    /// Creates a registrar for the replicator service.
    DECLARE_SERVICE_REGISTRAR(Replicator)(std::shared_ptr<storage::ReplicatorService> pReplicatorService);
}}
