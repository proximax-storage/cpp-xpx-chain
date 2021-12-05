/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ReplicatorService.h"
#include "catapult/extensions/ServiceLocator.h"

namespace catapult { namespace storage {

    // region - replicator service registrar

    namespace {
        constexpr auto Service_Name = "replicator";
		constexpr auto Replicator_Host = "127.0.0.1:";

        class ReplicatorServiceRegistrar: public extensions::ServiceRegistrar {
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
				auto& storageState = state.pluginManager().storageState();
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
        Impl(crypto::KeyPair& keyPair, extensions::ServiceState& serviceState)
            : m_keyPair(keyPair)
			, m_serviceState(serviceState)
		{}

    public:
		void init(const StorageConfiguration& storageConfig) {
			const auto& immutableConfig = m_serviceState.config().Immutable;
			TransactionSender transactionSender(immutableConfig, storageConfig,
												m_serviceState.hooks().transactionRangeConsumerFactory()(disruptor::InputSource::Local));
			auto& storageState = m_serviceState.pluginManager().storageState();
			m_pReplicatorEventHandler = CreateReplicatorEventHandler(std::move(transactionSender), storageState, m_transactionStatusHandler);

			m_pReplicator = sirius::drive::createDefaultReplicator(
				reinterpret_cast<sirius::crypto::KeyPair&>(m_keyPair), // TODO: pass private key string.
				Replicator_Host,
				std::string(storageConfig.Port), // TODO: do not use move semantics.
				std::string(storageConfig.StorageDirectory), // TODO: do not use move semantics.
				std::string(storageConfig.SandboxDirectory), // TODO: do not use move semantics.
				storageConfig.UseTcpSocket,
				*m_pReplicatorEventHandler, // TODO: pass unique_ptr instead of ref.
				nullptr,
				Service_Name);

			auto drives = storageState.getReplicatorDrives(m_keyPair.publicKey());
			for (const auto& drive : drives) {
				addDrive(
					drive.Id,
					drive.Size,
					drive.UsedSize,
					drive.Replicators
				);

				for (const auto& dataModification : drive.DataModifications) {
					addDriveModification(
						drive.Id,
						dataModification.DownloadDataCdi,
						dataModification.Id,
						dataModification.Owner,
						dataModification.ExpectedUploadSize
					);
				}
			}

			auto channels = storageState.getDownloadChannels();
			for (const auto& channel : channels) {
				addDriveChannel(
					channel.Id,
					Key{}, // TODO add real drive key in V3
					channel.DownloadSize,
					channel.Consumers
				);
			}
		}

        void addDriveModification(
				const Key& driveKey,
				const Hash256& downloadDataCdi,
				const Hash256& modificationId,
				const Key& owner,
				uint64_t dataSize) {
            auto modifyRequest = sirius::drive::ModifyRequest{
                (const sirius::Hash256&) downloadDataCdi,
                (const sirius::Hash256&) modificationId,
                dataSize,
                {}, // TODO add replicators addresses
                (const sirius::Key&) owner
            };

            CATAPULT_LOG(debug) << "new modify request " << modifyRequest.m_transactionHash
                                << " for drive " << driveKey;

            m_pReplicator->asyncModify((const sirius::Key&) driveKey, std::move(modifyRequest));
       }

        void removeDriveModification(const Key& driveKey, const Hash256& transactionHash) {
            CATAPULT_LOG(debug) << "remove modify request " << transactionHash.data() << " for drive " << driveKey;

            m_pReplicator->asyncCancelModify(
                    (const sirius::Key&) driveKey,
                    (const sirius::Hash256&) transactionHash
            );
        }

        void addDriveChannel(
				const Hash256& channelId,
				const Key& driveKey,
				size_t prepaidDownloadSize,
				const std::vector<Key>& consumers) {
            CATAPULT_LOG(debug) << "add download channel " << channelId.data();

            std::vector<sirius::Key> castedConsumers;
            std::transform(
					consumers.begin(),
					consumers.end(),
					castedConsumers.begin(),
					[](const Key& key) {return (const sirius::Key&) key;}
			);

            auto downloadRequest = sirius::drive::DownloadRequest{
                    (const std::array<uint8_t, 32>&) channelId,
                    prepaidDownloadSize,
                    {}, // TODO add replicators addresses
                    castedConsumers
            };

            m_pReplicator->asyncAddDownloadChannelInfo((const sirius::Key&) driveKey, std::move(downloadRequest));
        }

		void increaseDownloadChannelSize(const Hash256& channelId, size_t downloadSize) {
			CATAPULT_LOG(debug) << "updating download  channel " << channelId.data();

			auto channel = m_serviceState.pluginManager().storageState().getDownloadChannel(const_cast<Hash256&>(channelId));
			catapult::storage::ReplicatorService::Impl::addDriveChannel(
					channelId,
					channel.DriveKey,
					channel.DownloadSize + downloadSize,
					channel.Consumers
			);
		}

        void closeDriveChannel(const Hash256& channelId) {
            CATAPULT_LOG(debug) << "closing download channel " << channelId.data();

            m_pReplicator->removeDownloadChannelInfo((const std::array<uint8_t, 32>&) channelId);
        }

        void addDrive(const Key& driveKey, uint64_t driveSize, uint64_t usedSize, const utils::KeySet& replicators) {
            CATAPULT_LOG(debug) << "add drive " << driveKey;

            sirius::drive::ReplicatorList replicatorList;
            replicatorList.reserve(replicators.size());

            auto i = 0;
            for (const auto& rep : replicators) {
                replicatorList.at(i).m_publicKey = (const sirius::Key&) rep;
                i++;
            }

            sirius::drive::AddDriveRequest request{driveSize, usedSize, replicatorList};
            m_pReplicator->asyncAddDrive((const sirius::Key&) driveKey, request);
        }

        bool containsDrive(const Key& driveKey) {
            return m_serviceState.pluginManager().storageState().isReplicatorBelongToDrive(reinterpret_cast<const Key&>(m_keyPair), driveKey);
        }

        void closeDrive(const Key& driveKey, const Hash256& transactionHash) {
            CATAPULT_LOG(debug) << "closing drive " << driveKey;

            m_pReplicator->asyncCloseDrive(
                    (const sirius::Key&) driveKey,
                    (const sirius::Hash256&) transactionHash
            );
        }

		void notifyTransactionStatus(const model::Transaction& transaction, const Height& height, const Hash256& hash, uint32_t status) {
			m_transactionStatusHandler.handle(transaction.Signature, hash, status);
		}

        void stop() {
			// TODO: stop replicator.
        }

    private:
        crypto::KeyPair& m_keyPair; // TODO: change to const ref.
		extensions::ServiceState& m_serviceState;
		std::shared_ptr<sirius::drive::Replicator> m_pReplicator;
		std::unique_ptr<sirius::drive::ReplicatorEventHandler> m_pReplicatorEventHandler;
		TransactionStatusHandler m_transactionStatusHandler = TransactionStatusHandler{};
    };

    // endregion

    // region - replicator service

	ReplicatorService::ReplicatorService(crypto::KeyPair&& keyPair, StorageConfiguration&& storageConfig)
		: m_keyPair(std::move(keyPair))
		, m_storageConfig(std::move(storageConfig))
	{}

    void ReplicatorService::start() {
		if (m_pImpl)
			CATAPULT_THROW_RUNTIME_ERROR("replicator service already started");

		m_pImpl = std::make_shared<ReplicatorService::Impl>(m_keyPair, *m_pServiceState);
		m_pImpl->init(m_storageConfig);
    }

	void ReplicatorService::stop() {
		if (m_pImpl) {
			m_pImpl->stop();
			m_pImpl = nullptr;
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

    void ReplicatorService::addDriveChannel(const Hash256& channelId, const Key& driveKey, size_t prepaidDownloadSize, const std::vector<Key>& consumers) {
        if (m_pImpl)
            m_pImpl->addDriveChannel(channelId, driveKey, prepaidDownloadSize, consumers);
    }

    void ReplicatorService::increaseDownloadChannelSize(const Hash256& channelId, size_t downloadSize) {
        if (m_pImpl)
            m_pImpl->increaseDownloadChannelSize(channelId, downloadSize);
    }

    void ReplicatorService::closeDriveChannel(const Hash256& channelId) {
        if (m_pImpl)
            m_pImpl->closeDriveChannel(channelId);
    }

    void ReplicatorService::addDrive(const Key& driveKey, uint64_t driveSize, uint64_t usedSize, const utils::KeySet& replicators) {
        if (m_pImpl)
            m_pImpl->addDrive(driveKey, driveSize, usedSize, replicators);
    }

    bool ReplicatorService::containsDrive(const Key& driveKey) {
        if (m_pImpl)
            m_pImpl->containsDrive(driveKey);

        return false;
    }

    void ReplicatorService::closeDrive(const Key& driveKey, const Hash256& transactionHash) {
        if (m_pImpl)
            m_pImpl->closeDrive(driveKey, transactionHash);
    }

	void ReplicatorService::notifyTransactionStatus(const model::Transaction& transaction, const Height& height, const Hash256& hash, uint32_t status) {
        if (m_pImpl)
            m_pImpl->notifyTransactionStatus(transaction, height, hash, status);
	}

    // endregion
}}
