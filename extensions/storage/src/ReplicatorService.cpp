/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ReplicatorService.h"
#include "ReplicatorEventHandler.h"
#include "TransactionSender.h"
#include "TransactionStatusHandler.h"
#include "catapult/extensions/ServiceLocator.h"
#include "catapult/extensions/ServiceState.h"
#include "catapult/io/BlockStorageCache.h"

namespace catapult { namespace storage {

    // region - replicator service registrar

    namespace {
        constexpr auto Service_Name = "replicator";

        class ReplicatorServiceRegistrar : public extensions::ServiceRegistrar {
        public:
            explicit ReplicatorServiceRegistrar(std::shared_ptr<storage::ReplicatorService> pReplicatorService)
                    : m_pReplicatorService(std::move(pReplicatorService)) {}

            extensions::ServiceRegistrarInfo info() const override {
                return {"Replicator", extensions::ServiceRegistrarPhase::Post_Range_Consumers};
            }

            void registerServiceCounters(extensions::ServiceLocator&) override {
                // no additional counters
            }

            void registerServices(extensions::ServiceLocator& locator, extensions::ServiceState& state) override {
                locator.registerRootedService(Service_Name, m_pReplicatorService);

				m_pReplicatorService->setServiceState(&state);

				const auto& storage = state.storage();
				auto lastBlockElementSupplier = [&storage]() {
					auto storageView = storage.view();
					return storageView.loadBlockElement(storageView.chainHeight());
				};
                auto& storageState = state.pluginManager().storageState();
				storageState.setLastBlockElementSupplier(lastBlockElementSupplier);

                if (storageState.isReplicatorRegistered(m_pReplicatorService->replicatorKey())) {
                    CATAPULT_LOG(debug) << "starting replicator service";
                    m_pReplicatorService->start();
                }

                m_pReplicatorService.reset();
            }

        private:
            std::shared_ptr<storage::ReplicatorService> m_pReplicatorService;
        };
    }

    DECLARE_SERVICE_REGISTRAR(Replicator)(std::shared_ptr<storage::ReplicatorService> pReplicatorService) {
        return std::make_unique<ReplicatorServiceRegistrar>(std::move(pReplicatorService));
    }

    // endregion

    // region - replicator service implementation

    class ReplicatorService::Impl : public std::enable_shared_from_this<ReplicatorService::Impl> {
    public:
        Impl(const crypto::KeyPair& keyPair,
             extensions::ServiceState& serviceState,
             state::StorageState& storageState,
			 const std::vector<ionet::Node>& bootstrapReplicators)
             : m_keyPair(keyPair)
             , m_serviceState(serviceState)
             , m_storageState(storageState)
             , m_bootstrapReplicators(bootstrapReplicators)
		 {}

    public:
        void start(const StorageConfiguration& storageConfig) {
			const auto& config = m_serviceState.config();
            TransactionSender transactionSender(
				m_keyPair,
				config.Immutable,
				storageConfig,
				m_serviceState.hooks().transactionRangeConsumerFactory()(disruptor::InputSource::Local),
				m_storageState);

            m_pReplicatorEventHandler = CreateReplicatorEventHandler(
				std::move(transactionSender),
				m_storageState,
				m_transactionStatusHandler);

			std::vector<sirius::drive::ReplicatorInfo> bootstrapReplicators;
			bootstrapReplicators.reserve(m_bootstrapReplicators.size());
			for (const auto& node : m_bootstrapReplicators) {
				boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::make_address(node.endpoint().Host), node.endpoint().Port);
				bootstrapReplicators.emplace_back(sirius::drive::ReplicatorInfo{ endpoint, node.identityKey().array() });
			}

            m_pReplicator = sirius::drive::createDefaultReplicator(
				reinterpret_cast<const sirius::crypto::KeyPair&>(m_keyPair), // TODO: pass private key string.
				std::string(storageConfig.Host), // TODO: do not use move semantics.
				std::string(storageConfig.Port), // TODO: do not use move semantics.
				std::string(storageConfig.StorageDirectory), // TODO: do not use move semantics.
				std::string(storageConfig.SandboxDirectory), // TODO: do not use move semantics.
				bootstrapReplicators,
				storageConfig.UseTcpSocket,
				*m_pReplicatorEventHandler, // TODO: pass unique_ptr instead of ref.
				nullptr,
				Service_Name);

			m_pReplicatorEventHandler->setReplicator(m_pReplicator);
			m_pReplicator->start();

            auto drives = m_storageState.getReplicatorDrives(m_keyPair.publicKey());
            for (const auto& drive: drives) {
                addDrive(drive.Id, drive.Size);

                auto pLastApprovedModification = m_storageState.getLastApprovedDataModification(drive.Id);
				if (!!pLastApprovedModification) {
					m_pReplicator->asyncApprovalTransactionHasBeenPublished(sirius::drive::ApprovalTransactionInfo{
						pLastApprovedModification->DriveKey.array(),
						pLastApprovedModification->Id.array(),
						pLastApprovedModification->DownloadDataCdi.array(),
						pLastApprovedModification->ExpectedUploadSize,
						pLastApprovedModification->ActualUploadSize - pLastApprovedModification->ExpectedUploadSize,
						pLastApprovedModification->UsedSize});
				}

				auto pActiveVerification = m_storageState.getActiveVerification(drive.Id);
				if (!!pActiveVerification) {
					std::vector<sirius::Key> replicators;
					sirius::Hash256 verificationTrigger(pActiveVerification->VerificationTrigger.array());
					sirius::drive::InfoHash rootHash(pActiveVerification->RootHash.array());
					replicators.reserve(pActiveVerification->Shards[0].size());

					for (uint32_t i = 0u; i < pActiveVerification->Shards.size(); ++i) {
						replicators.clear();
						const auto& shard = pActiveVerification->Shards[i];
						std::for_each(shard.begin(), shard.end(), [&replicators](const auto& key) { replicators.push_back(key.array()); });

						m_pReplicator->asyncStartDriveVerification(
							drive.Id.array(),
							sirius::drive::VerificationRequest{
								verificationTrigger,
								i,
								rootHash,
								replicators});
					}
				}

                for (const auto& dataModification: drive.DataModifications) {
                    addDriveModification(
						drive.Id,
						dataModification.DownloadDataCdi,
						dataModification.Id,
						dataModification.Owner,
						dataModification.ExpectedUploadSize);
                }
            }

            auto channels = m_storageState.getDownloadChannels(m_keyPair.publicKey());
            for (const auto& channel: channels) {
                addDownloadChannel(
					channel.Id,
					channel.DriveKey,
					channel.DownloadSize,
					channel.Consumers,
					channel.Replicators);
            }

			m_pReplicator->asyncInitializationFinished();
        }

        void addDriveModification(
                const Key& driveKey,
                const Hash256& downloadDataCdi,
                const Hash256& modificationId,
                const Key& owner,
                uint64_t dataSize) {
            CATAPULT_LOG(debug) << "new modify request " << modificationId << " for drive " << driveKey;

            auto replicators = castReplicatorKeys(m_storageState.getDriveReplicators(driveKey));
            m_pReplicator->asyncModify(
				driveKey.array(),
				sirius::drive::ModifyRequest{
					downloadDataCdi.array(),
					modificationId.array(),
					dataSize,
					replicators,
					owner.array()});
        }

        void removeDriveModification(const Key& driveKey, const Hash256& transactionHash) {
            CATAPULT_LOG(debug) << "remove modify request " << transactionHash.data() << " for drive " << driveKey;

            m_pReplicator->asyncCancelModify(driveKey.array(), transactionHash.array());
        }

        void addDownloadChannel(
                const Hash256& channelId,
                const Key& driveKey,
                size_t prepaidDownloadSize,
                const std::vector<Key>& consumers,
                const std::vector<Key>& replicators) {
            CATAPULT_LOG(debug) << "add download channel " << channelId.data();

			std::vector<sirius::Key> castedReplicators;
			castedReplicators.reserve(replicators.size());
			std::for_each(replicators.begin(), replicators.end(), [&castedReplicators](const auto& key) { castedReplicators.push_back(key.array()); });

			std::vector<sirius::Key> castedConsumers;
			castedConsumers.reserve(consumers.size());
			std::for_each(consumers.begin(), consumers.end(), [&castedConsumers](const auto& key) { castedConsumers.push_back(key.array()); });

            m_pReplicator->asyncAddDownloadChannelInfo(
				driveKey.array(),
				sirius::drive::DownloadRequest{
					channelId.array(),
					prepaidDownloadSize,
					castedReplicators,
					castedConsumers});
        }

        void addDownloadChannel(const Hash256& channelId) {
            auto pChannel = m_storageState.getDownloadChannel(m_keyPair.publicKey(), channelId);
			if (!!pChannel) {
				addDownloadChannel(
					channelId,
					pChannel->DriveKey,
					pChannel->DownloadSize,
					pChannel->Consumers,
					pChannel->Replicators);
			}
        }

        void increaseDownloadChannelSize(const Hash256& channelId, size_t downloadSize) {
            CATAPULT_LOG(debug) << "updating download channel " << channelId.data();

            auto pChannel = m_storageState.getDownloadChannel(m_keyPair.publicKey(), channelId);
			if (!!pChannel) {
				addDownloadChannel(
					channelId,
					pChannel->DriveKey,
					pChannel->DownloadSize + downloadSize,
					pChannel->Consumers,
					pChannel->Replicators);
			}
        }

        void closeDownloadChannel(const Hash256& channelId) {
            CATAPULT_LOG(debug) << "closing download channel " << channelId.data();

            auto pChannel = m_storageState.getDownloadChannel(m_keyPair.publicKey(), channelId);
			if (!!pChannel) {
				m_pReplicator->asyncInitiateDownloadApprovalTransactionInfo(m_storageState.lastBlockElementSupplier()()->EntityHash.array(), channelId.array());
				m_pReplicator->asyncRemoveDownloadChannelInfo(pChannel->DriveKey.array(), channelId.array());
			}
        }

        void addDrive(const Key& driveKey, uint64_t driveSize) {
            CATAPULT_LOG(debug) << "add drive " << driveKey;

            auto replicators = castReplicatorKeys(m_storageState.getDriveReplicators(driveKey));
            auto downloadWork = m_storageState.getDownloadWork(m_keyPair.publicKey(), driveKey);
            sirius::drive::AddDriveRequest request{driveSize, downloadWork, replicators, m_storageState.getDrive(driveKey).Owner.array()};
            m_pReplicator->asyncAddDrive(driveKey.array(), request);
        }

        bool isAssignedToDrive(const Key& driveKey) {
			return m_storageState.isReplicatorAssignedToDrive(m_keyPair.publicKey(), driveKey);
        }

        void closeDrive(const Key& driveKey, const Hash256& transactionHash) {
            CATAPULT_LOG(debug) << "closing drive " << driveKey;

            m_pReplicator->asyncCloseDrive(
				driveKey.array(),
				transactionHash.array());
        }

        void maybeCancelVerifications() {
			auto verifications = m_storageState.getActiveVerifications(m_keyPair.publicKey());
			for (const auto& verification : verifications) {
				if (verification.Expired) {
					sirius::Hash256 verificationTrigger = verification.VerificationTrigger.array();
					m_pReplicator->asyncCancelDriveVerification(
						verification.DriveKey.array(),
						std::move(verificationTrigger));
				}
			}
        }

        void dataModificationApprovalPublished(
                const Key& driveKey,
                const Hash256& modificationId,
                const Hash256& rootHash,
                std::vector<Key>& replicators) {
			std::vector<std::array<uint8_t, 32>> replicatorKeys;
			for (const auto& key : replicators)
				replicatorKeys.push_back(key.array());
            m_pReplicator->asyncApprovalTransactionHasBeenPublished(sirius::drive::PublishedModificationApprovalTransactionInfo{
				driveKey.array(),
				modificationId.array(),
				rootHash.array(),
				replicatorKeys});
        }

        void dataModificationSingleApprovalPublished(const Key& driveKey, const Hash256& modificationId) {
            m_pReplicator->asyncSingleApprovalTransactionHasBeenPublished(sirius::drive::PublishedModificationSingleApprovalTransactionInfo{
				driveKey.array(),
				modificationId.array()});
        }

        void downloadApprovalPublished(const Hash256& approvalTrigger, const Hash256& downloadChannelId) {
			auto pDownloadChannel = m_storageState.getDownloadChannel(m_keyPair.publicKey(), downloadChannelId);
			if (!pDownloadChannel)
				return;

			auto driveClosed = !m_storageState.driveExist(pDownloadChannel->DriveKey);
            m_pReplicator->asyncDownloadApprovalTransactionHasBeenPublished(approvalTrigger.array(), downloadChannelId.array(), driveClosed);
        }

        void notifyTransactionStatus(const Hash256& hash, uint32_t status) {
            m_transactionStatusHandler.handle(hash, status);
        }

        void stop() {
			m_pReplicator.reset();
        }

    private:
        std::vector<sirius::Key> castReplicatorKeys(const std::vector<Key>& keys) {
            std::vector<sirius::Key> replicators;
			replicators.reserve(keys.size());
            for (const auto& key: keys)
				replicators.emplace_back(key.array());

			return replicators;
        }

    private:
        const crypto::KeyPair& m_keyPair;
        extensions::ServiceState& m_serviceState;
        state::StorageState& m_storageState;

        std::shared_ptr<sirius::drive::Replicator> m_pReplicator;
        std::unique_ptr<ReplicatorEventHandler> m_pReplicatorEventHandler;
        TransactionStatusHandler m_transactionStatusHandler;
		const std::vector<ionet::Node>& m_bootstrapReplicators;
    };

    // endregion

    // region - replicator service

    ReplicatorService::ReplicatorService(crypto::KeyPair&& keyPair, StorageConfiguration&& storageConfig, std::vector<ionet::Node>&& bootstrapReplicators)
		: m_keyPair(std::move(keyPair))
		, m_storageConfig(std::move(storageConfig))
		, m_bootstrapReplicators(std::move(bootstrapReplicators))
	{}

	ReplicatorService::~ReplicatorService() {
		stop();
	}

    void ReplicatorService::start() {
        if (m_pImpl)
			CATAPULT_THROW_RUNTIME_ERROR("replicator service already started");

        m_pImpl = std::make_unique<ReplicatorService::Impl>(
			m_keyPair,
			*m_pServiceState,
			m_pServiceState->pluginManager().storageState(),
			m_bootstrapReplicators);
        m_pImpl->start(m_storageConfig);
    }

    void ReplicatorService::stop() {
        if (m_pImpl) {
            m_pImpl->stop();
            m_pImpl.reset();
        }
    }

    const Key& ReplicatorService::replicatorKey() const {
        return m_keyPair.publicKey();
    }

    bool ReplicatorService::isReplicatorRegistered(const Key& key) {
        return m_pServiceState->pluginManager().storageState().isReplicatorRegistered(key);
    }

    void ReplicatorService::addDriveModification(
            const Key& driveKey,
            const Hash256& downloadDataCdi,
            const Hash256& modificationId,
            const Key& owner,
            uint64_t dataSize) {
        if (m_pImpl)
			m_pImpl->addDriveModification(driveKey, downloadDataCdi, modificationId, owner, dataSize);
    }

    void ReplicatorService::removeDriveModification(const Key& driveKey, const Hash256& dataModificationId) {
        if (m_pImpl)
            m_pImpl->removeDriveModification(driveKey, dataModificationId);
    }

    void ReplicatorService::addDownloadChannel(const Hash256& channelId) {
        if (m_pImpl)
            m_pImpl->addDownloadChannel(channelId);
    }

    void ReplicatorService::increaseDownloadChannelSize(const Hash256& channelId, size_t downloadSize) {
        if (m_pImpl)
            m_pImpl->increaseDownloadChannelSize(channelId, downloadSize);
    }

    void ReplicatorService::closeDownloadChannel(const Hash256& channelId) {
        if (m_pImpl)
            m_pImpl->closeDownloadChannel(channelId);
    }

    void ReplicatorService::addDrive(const Key& driveKey, uint64_t driveSize) {
        if (m_pImpl)
            m_pImpl->addDrive(driveKey, driveSize);
    }

    bool ReplicatorService::isAssignedToDrive(const Key& driveKey) {
        if (m_pImpl)
            return m_pImpl->isAssignedToDrive(driveKey);

        return false;
    }

    void ReplicatorService::closeDrive(const Key& driveKey, const Hash256& transactionHash) {
        if (m_pImpl)
            m_pImpl->closeDrive(driveKey, transactionHash);
    }

    void ReplicatorService::maybeCancelVerifications() {
        if (m_pImpl)
            m_pImpl->maybeCancelVerifications();
    }

    void ReplicatorService::notifyTransactionStatus(const Hash256& hash, uint32_t status) {
        if (m_pImpl)
            m_pImpl->notifyTransactionStatus(hash, status);
    }

    void ReplicatorService::dataModificationApprovalPublished(
            const Key& driveKey,
            const Hash256& modificationId,
            const Hash256& rootHash,
            std::vector<Key>& replicators) {
        if (m_pImpl)
            m_pImpl->dataModificationApprovalPublished(driveKey, modificationId, rootHash, replicators);
    }

    void ReplicatorService::dataModificationSingleApprovalPublished(const Key& driveKey, const Hash256& modificationId) {
        if (m_pImpl)
            m_pImpl->dataModificationSingleApprovalPublished(driveKey, modificationId);
    }

    void ReplicatorService::downloadApprovalPublished(const Hash256& approvalTrigger, const Hash256& downloadChannelId) {
        if (m_pImpl)
            m_pImpl->downloadApprovalPublished(approvalTrigger, downloadChannelId);
    }

    // endregion
}}
