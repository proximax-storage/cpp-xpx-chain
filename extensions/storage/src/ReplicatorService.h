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
#include "catapult/ionet/Node.h"

namespace catapult { namespace model { class Transaction; } }

namespace catapult { namespace storage {

    class ReplicatorService {
    public:
        ReplicatorService(crypto::KeyPair&& keyPair, StorageConfiguration&& storageConfig, std::vector<ionet::Node>&& bootstrapReplicators);
		~ReplicatorService();

    public:
        void start();
        void stop();

        void setServiceState(extensions::ServiceState* pServiceState) {
            m_pServiceState = pServiceState;
        }

        const Key& replicatorKey() const;

        bool isReplicatorRegistered(const Key& key);

		void addDriveModification(const Key& driveKey, const Hash256& downloadDataCdi, const Hash256& modificationId, const Key& owner, uint64_t dataSizeMegabytes);
        void removeDriveModification(const Key& driveKey, const Hash256& dataModificationId);

        void addDownloadChannel(const Hash256& channelId);
        void increaseDownloadChannelSize(const Hash256& channelId);
        void initiateDownloadApproval(const Hash256& channelId, const Hash256& eventHash);

		void addDrive(const Key& driveKey);
		void removeDrive(const Key& driveKey);
        bool isAssignedToDrive(const Key& driveKey);
        bool isAssignedToChannel(const Hash256& channelId);
        void closeDrive(const Key& driveKey, const Hash256& transactionHash);
        std::optional<Height> driveAddedAt(const Key& driveKey);
        std::optional<Height> channelAddedAt(const Hash256& driveKey);

        void exploreNewDrives();

        void processVerifications(const Hash256& blockHash);

	public:
		void updateDriveReplicators(const Key& driveKey);
		void updateShardDonator(const Key& driveKey);
		void updateShardRecipient(const Key& driveKey);
		void updateDriveDownloadChannels(const Key& driveKey);

    public:
    	void anotherReplicatorOnboarded(const Key& replicatorKey);
		void dataModificationApprovalPublished(const Key& driveKey, const Hash256& modificationId, const Hash256& rootHash, std::vector<Key>& replicators);
        void dataModificationSingleApprovalPublished(const Key& driveKey, const Hash256& modificationId);
        void downloadApprovalPublished(const Hash256& approvalTrigger, const Hash256& downloadChannelId);
		void endDriveVerificationPublished(const Key& driveKey, const Hash256& verificationTrigger);

    public:
        void notifyTransactionStatus(const Hash256& hash, uint32_t status);

    private:
        class Impl;

        std::unique_ptr<Impl> m_pImpl;

        crypto::KeyPair m_keyPair;
        StorageConfiguration m_storageConfig;
        extensions::ServiceState* m_pServiceState;
		std::vector<ionet::Node> m_bootstrapReplicators;
    };

    /// Creates a registrar for the replicator service.
    DECLARE_SERVICE_REGISTRAR(Replicator)(std::shared_ptr<storage::ReplicatorService> pReplicatorService);
}}
