/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "StorageConfiguration.h"
#include "TransactionSender.h"
#include "catapult/types.h"
#include "catapult/crypto/KeyPair.h"
#include "catapult/extensions/ServiceRegistrar.h"
#include "catapult/ionet/Node.h"
#include "catapult/utils/ArraySet.h"
#include "drive/Replicator.h"
#include <optional>
#include <boost/asio.hpp>

namespace catapult {
	namespace model { class Transaction; }
	namespace state {
		class Drive;
		class DownloadChannel;
		class DriveVerification;
	}
	namespace thread { class IoThreadPool; }
	namespace storage { class TransactionStatusHandler; }
}

namespace catapult { namespace storage {

    class ReplicatorService : public std::enable_shared_from_this<ReplicatorService>, public sirius::drive::ReplicatorEventHandler {
    public:
        ReplicatorService(StorageConfiguration&& storageConfig, std::vector<ionet::Node>&& bootstrapReplicators);
		~ReplicatorService();

    public:
		bool registered();
		void markRegistered();
		bool isAlive();
		bool initialized();
        void start();
        void stop();
		void shutdown();

        void setServiceState(extensions::ServiceState* pServiceState);
        void setThreadPool(const std::shared_ptr<thread::IoThreadPool>& pPool);

        const Key& replicatorKey() const;
		std::shared_ptr<TransactionStatusHandler> transactionStatusHandler();
		std::shared_ptr<sirius::drive::Replicator> replicator();
		std::shared_ptr<state::Drive> getDrive(const Key& driveKey);
		std::shared_ptr<state::DownloadChannel> getDownloadChannel(const Hash256& channelId);

		void addDrive(const std::shared_ptr<state::Drive>& pDrive);
		void updateDrive(const std::shared_ptr<state::Drive>& pUpdatedDrive);
		void updateDrives(const std::vector<std::shared_ptr<state::Drive>>& drives);
		void removeDrive(const Key& driveKey);
        void closeDrive(const Key& driveKey, const Hash256& transactionHash);

        bool driveAdded(const Key& driveKey);
        bool channelAdded(const std::shared_ptr<state::DownloadChannel>& pChannel);

		void removeDriveModification(const std::shared_ptr<state::Drive>& pDrive, const Hash256& dataModificationId);
		void addDownloadChannel(const std::shared_ptr<state::DownloadChannel>& pChannel);
		void increaseDownloadChannelSize(const std::shared_ptr<state::DownloadChannel>& pChannel);
		void initiateDownloadApproval(const std::vector<std::shared_ptr<state::DownloadChannel>>& channels);
		void processVerifications(const std::unordered_map<Key, std::shared_ptr<state::DriveVerification>, utils::ArrayHasher<Key>>& verifications);

    public:
    	void addDriveModification(const Key& driveKey, const Hash256& downloadDataCdi, const Hash256& modificationId, const Key& owner, uint64_t dataSizeMegabytes, const utils::KeySet& replicators);
		void startStream(const std::shared_ptr<state::Drive>& pDrive, const Hash256& streamId, const std::string& folder, uint64_t expectedSizeMegabytes);
		void streamPaymentPublished(const std::shared_ptr<state::Drive>& pDrive, const Hash256& streamId);
		void finishStream(const std::shared_ptr<state::Drive>& pDrive, const Hash256& streamId, const Hash256& finishDownloadDataCdi, uint64_t actualSizeMegabytes);
		void dataModificationApprovalPublished(const std::shared_ptr<state::Drive>& pDrive, const Hash256& modificationId, const Hash256& rootHash, const std::vector<Key>& replicators);
        void dataModificationSingleApprovalPublished(const std::shared_ptr<state::Drive>& pDrive, const Hash256& modificationId);
        void downloadApprovalPublished(const std::shared_ptr<state::DownloadChannel>& pChannel, bool downloadChannelClosed);
		void endDriveVerificationPublished(const Key& driveKey, const Hash256& verificationTrigger);

	public:
        void notifyTransactionStatus(const Hash256& hash, uint32_t status);

	public:
		void post(const consumer<const std::shared_ptr<ReplicatorService>&>& handler);

	public:
		void modifyApprovalTransactionIsReady(sirius::drive::Replicator&, const sirius::drive::ApprovalTransactionInfo& info) override;
		void singleModifyApprovalTransactionIsReady(sirius::drive::Replicator&, const sirius::drive::ApprovalTransactionInfo& info) override;
		void downloadApprovalTransactionIsReady(sirius::drive::Replicator&, const sirius::drive::DownloadApprovalTransactionInfo& info) override;
		void verificationTransactionIsReady(sirius::drive::Replicator&, const sirius::drive::VerifyApprovalTxInfo& info) override;
		void opinionHasBeenReceived(sirius::drive::Replicator&, const sirius::drive::ApprovalTransactionInfo& info) override;
		void downloadOpinionHasBeenReceived(sirius::drive::Replicator&, const sirius::drive::DownloadApprovalTransactionInfo& info) override;
		void onLibtorrentSessionError(const std::string& message) override;

    private:
        class Impl;
        std::unique_ptr<Impl> m_pImpl;

        crypto::KeyPair m_keyPair;
        StorageConfiguration m_storageConfig;
        extensions::ServiceState* m_pServiceState;
		std::vector<ionet::Node> m_bootstrapReplicators;
		bool m_registered;
		std::shared_ptr<thread::IoThreadPool> m_pPool;
		std::unique_ptr<boost::asio::io_context::strand> m_pStrand;
		std::unique_ptr<TransactionSender> m_pTransactionSender;
    };

    /// Creates a registrar for the replicator service.
    DECLARE_SERVICE_REGISTRAR(Replicator)(std::shared_ptr<storage::ReplicatorService> pReplicatorService);
}}
