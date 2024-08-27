/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "extensions/storage/src/StorageConfiguration.h"
#include "catapult/notification_handlers/HandlerContext.h"
#include "catapult/types.h"
#include "catapult/crypto/KeyPair.h"
#include "catapult/extensions/ServiceRegistrar.h"
#include "catapult/ionet/Node.h"
#include <optional>

namespace catapult { namespace model { class Transaction; } }

namespace catapult { namespace storage {

    class ReplicatorService {
    public:
        ReplicatorService(StorageConfiguration&& storageConfig, std::vector<ionet::Node>&& bootstrapReplicators);
		~ReplicatorService();

    public:
        void start(const cache::ReadOnlyCatapultCache& cache);
        void stop();
		void restart(const cache::ReadOnlyCatapultCache& cache);
		void maybeRestart(const cache::ReadOnlyCatapultCache& cache);

        void setServiceState(extensions::ServiceState* pServiceState) {
            m_pServiceState = pServiceState;
        }

        const Key& replicatorKey() const;

        bool isReplicatorRegistered(const Key& key, const cache::ReadOnlyCatapultCache& cache);

		void removeDriveModification(const Key& driveKey, const Hash256& dataModificationId);

        void addDownloadChannel(const Hash256& channelId, const cache::ReadOnlyCatapultCache& cache);
        void increaseDownloadChannelSize(const Hash256& channelId, const cache::ReadOnlyCatapultCache& cache);
        void initiateDownloadApproval(const Hash256& channelId, const Hash256& eventHash, const cache::ReadOnlyCatapultCache& cache);

		void addDrive(const Key& driveKey, const cache::ReadOnlyCatapultCache& cache);
		void removeDrive(const Key& driveKey);
        bool isAssignedToDrive(const Key& driveKey, const cache::ReadOnlyCatapultCache& cache);
        bool isAssignedToChannel(const Hash256& channelId, const cache::ReadOnlyCatapultCache& cache);
        void closeDrive(const Key& driveKey, const Hash256& transactionHash);
        std::optional<Height> driveAddedAt(const Key& driveKey);
        std::optional<Height> channelAddedAt(const Hash256& channelId);

        void processVerifications(const Hash256& eventHash, const Timestamp& timestamp, const cache::ReadOnlyCatapultCache& cache);

	public:
		void updateDriveReplicators(const Key& driveKey, const cache::ReadOnlyCatapultCache& cache);
		void updateShardDonator(const Key& driveKey, const cache::ReadOnlyCatapultCache& cache);
		void updateShardRecipient(const Key& driveKey, const cache::ReadOnlyCatapultCache& cache);
		void updateDriveDownloadChannels(const Key& driveKey, const cache::ReadOnlyCatapultCache& cache);
		void updateReplicatorDownloadChannels(const cache::ReadOnlyCatapultCache& cache);
		void updateReplicatorDrives(const Hash256& eventHash, const cache::ReadOnlyCatapultCache& cache);
		void exploreNewReplicatorDrives(const cache::ReadOnlyCatapultCache& cache);

    public:
    	void anotherReplicatorOnboarded(const Key& replicatorKey, const cache::ReadOnlyCatapultCache& cache);
    	void downloadBlockPublished(const Hash256& eventHash, const cache::ReadOnlyCatapultCache& cache);
    	void addDriveModification(const Key& driveKey, const Hash256& downloadDataCdi, const Hash256& modificationId,
								  const Key& owner, uint64_t dataSizeMegabytes, const cache::ReadOnlyCatapultCache& cache);
		void startStream(const Key& driveKey, const Hash256& streamId, const Key& streamerKey,
						 const std::string& folder, uint64_t expectedSizeMegabytes, const cache::ReadOnlyCatapultCache& cache);
		void streamPaymentPublished(const Key& driveKey, const Hash256& streamId, const cache::ReadOnlyCatapultCache& cache);
		void finishStream(const Key& driveKey, const Hash256& streamId, const Hash256& finishDownloadDataCdi, uint64_t actualSizeMegabytes);
		void dataModificationApprovalPublished(const Key& driveKey, const Hash256& modificationId, const Hash256& rootHash, std::vector<Key>& replicators);
        void dataModificationSingleApprovalPublished(const Key& driveKey, const Hash256& modificationId);
        void downloadApprovalPublished(const Hash256& approvalTrigger, const Hash256& downloadChannelId, const cache::ReadOnlyCatapultCache& cache);
		void endDriveVerificationPublished(const Key& driveKey, const Hash256& verificationTrigger);

	public:
		bool driveExists(const Key& driveKey, const cache::ReadOnlyCatapultCache& cache);
		bool channelExists(const Hash256& channelId, const cache::ReadOnlyCatapultCache& cache);

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
