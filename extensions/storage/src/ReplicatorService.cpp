/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ReplicatorService.h"
#include <utility>
#include "StoragePacketHandlers.h"
#include "sdk/src/extensions/TransactionExtensions.h"
#include "catapult/utils/NetworkTime.h"
#include "catapult/extensions/ServiceState.h"
#include "catapult/extensions/ServiceLocator.h"
#include "drive/Replicator.h"
#include "catapult/state/StorageState.h"

namespace catapult { namespace storage {

    // region - replicator service registrar

    namespace {
        constexpr auto Service_Name = "replicator";

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

                m_pReplicatorService->setTransactionRangeHandler(state.hooks().transactionRangeConsumerFactory()(disruptor::InputSource::Local));
                RegisterStartDownloadFilesHandler(m_pReplicatorService, state.packetHandlers());
                RegisterStopDownloadFilesHandler(m_pReplicatorService, state.packetHandlers());

                auto& storageState = state.pluginManager().getStorageState();
                if (storageState.isReplicatorRegistered(m_pReplicatorService->replicatorKey())) {
                    CATAPULT_LOG(debug) << "starting replicator service";
                    m_pReplicatorService->start();

                    auto replicatorData = storageState.getReplicatorData(m_pReplicatorService->replicatorKey(), state.cache());
                    for (const auto& driveState : replicatorData.DrivesStates) {
                        m_pReplicatorService->addDrive(
                                driveState.DriveKey,
                                driveState.Size,
                                driveState.UsedSize,
                                driveState.Replicators
                        );

                        const auto& dataModification = driveState.LastDataModification;
                        if (dataModification.Id == Hash256{}) {
                            m_pReplicatorService->addDriveModification(
                                    driveState.DriveKey,
                                    dataModification.DownloadDataCdi,
                                    dataModification.Id,
                                    dataModification.Owner,
                                    dataModification.ExpectedUploadSize
                            );
                        }
                    }

                    for (const auto& channel : replicatorData.DownloadChannels) {
                        // TODO is a consumer is already in ListOfPublicKeys
                        auto consumers = channel.ListOfPublicKeys;
                        consumers.emplace_back(channel.Consumer);

                        m_pReplicatorService->addDriveChannel(
                                Hash256{},
                                Key{}, // TODO add real drive key in V3
                                channel.DownloadSize,
                                consumers
                        );
                    }
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

    // region - replicator service

    namespace {
        constexpr auto Replicator_Host = "127.0.0.1:";

        auto AsString(const Key& driveKey) {
            std::ostringstream out;
            out << driveKey;
            return out.str();
        }
    }

    class ReplicatorService::Impl : public std::enable_shared_from_this<ReplicatorService::Impl> {
    public:
        Impl(
                sirius::crypto::KeyPair&& keyPair,
                model::NetworkIdentifier networkIdentifier,
                const GenerationHash& generationHash,
                StorageConfiguration storageConfig,
                handlers::TransactionRangeHandler transactionRangeHandler,
                state::StorageState& storageState)
                : m_stopped(false)
                , m_keyPair(std::move(keyPair))
                , m_storageState(storageState)
                , m_transactionSender(TransactionSender(
                        networkIdentifier,
                        generationHash,
                        std::move(transactionRangeHandler),
                        storageConfig
                ))
                , m_eventHandler(ReplicatorEventHandler(m_transactionSender, m_storageState))
                , m_pReplicator(sirius::drive::createDefaultReplicator(
                        keyPair,
                        Replicator_Host,
                        std::move(storageConfig.Port),
                        std::move(storageConfig.StorageDirectory),
                        std::move(storageConfig.SandboxDirectory),
                        storageConfig.UseTcpSocket,
                        m_eventHandler,
                        nullptr,
                        Service_Name
                ))
        {}

    public:
        Key replicatorKey() const {
            Key convertedKey;
            std::copy(m_keyPair.publicKey().begin(), m_keyPair.publicKey().end(), convertedKey.begin());

            return convertedKey;
        }

        void addDriveModification(const Key& driveKey,
                                  const Hash256& downloadDataCdi,
                                  const Hash256& modificationId,
                                  const Key& owner,
                                  uint64_t dataSize
       ) {
            if (m_stopped)
                return;

            auto modifyRequest = sirius::drive::ModifyRequest{
                (const sirius::Hash256&) downloadDataCdi,
                (const sirius::Hash256&) modificationId,
                dataSize,
                {}, // TODO add replicators addresses
                (const sirius::Key&) owner
            };

//            CATAPULT_LOG(debug) << "new modify request for " << driveKey << "drive: \n\t"
//                                << modifyRequest;

            m_pReplicator->asyncModify((const sirius::Key&) driveKey, std::move(modifyRequest));
       }

        void removeDriveModification(const Key& driveKey, const Hash256& transactionHash) {
            if (m_stopped)
                return;

//            CATAPULT_LOG(debug) << "new modify request for " << driveKey << "drive: \n"
//                                << "\ttransactions hash:" << transactionHash;

            m_pReplicator->asyncCancelModify(
                    (const sirius::Key&) driveKey,
                    (const sirius::Hash256&) transactionHash
            );
        }

        void addDriveChannel(const Hash256& channelId,
                             const Key& driveKey,
                             size_t prepaidDownloadSize,
                             const std::vector<Key>& consumers
        ) {
            if (m_stopped)
                return;

//            CATAPULT_LOG(debug) << "add download channel " << channelId;

            std::vector<sirius::Key> castedConsumers;
            std::transform(consumers.begin(), consumers.end(), castedConsumers.begin(), [](const Key& key) {return (const sirius::Key&) key;});

            auto downloadRequest = sirius::drive::DownloadRequest{
                    (const std::array<uint8_t, 32>&) channelId,
                    prepaidDownloadSize,
                    {}, // TODO add replicators addresses
                    castedConsumers
            };

            m_pReplicator->asyncAddDownloadChannelInfo((const sirius::Key&) driveKey, std::move(downloadRequest));
        }

        void increaseDownloadChannelSize(const Hash256& channelId, size_t increasedDownloadSize) {
//            catapult::storage::ReplicatorService::Impl::addDrive()
        }

        void closeDriveChannel(const Hash256& channelId) {
            if (m_stopped)
                return;

//            CATAPULT_LOG(debug) << "closing download channel " << channelId;

            m_pReplicator->removeDownloadChannelInfo((const std::array<uint8_t, 32>&) channelId);
        }

        void addDrive(const Key& driveKey, uint64_t driveSize, uint64_t usedSize, const utils::KeySet& replicators) {
            if (m_stopped)
                return;

//            CATAPULT_LOG(debug) << "add drive " << driveKey;

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

        bool driveExist(const Key& driveKey) {
            if (m_stopped)
                return false;

//            m_pReplicator.findDrive(driveKey)
            return true;
        }

        void closeDrive(const Key& driveKey, const Hash256& transactionHash) {
            if (m_stopped)
                return;

//            CATAPULT_LOG(debug) << "closing drive " << driveKey;

            m_pReplicator->asyncCloseDrive(
                    (const sirius::Key&) driveKey,
                    (const sirius::Hash256&) transactionHash
            );
        }

        void shutdown() {
            if (m_stopped)
                return;

            m_stopped = true;
        }

        void terminate() {
            shutdown();
            // TODO close all drives
        }

    private:
        bool m_stopped;
        sirius::crypto::KeyPair m_keyPair;
        state::StorageState& m_storageState;
        TransactionSender m_transactionSender;
        ReplicatorEventHandler m_eventHandler;
        std::shared_ptr<sirius::drive::Replicator> m_pReplicator;
    };

    void ReplicatorService::start() {
        if (!m_pImpl) {
//            sirius::crypto::PrivateKey convertedPrivateKey;
//            std::copy(m_keyPair.privateKey().begin(), m_keyPair.privateKey().end(), convertedPrivateKey.begin());

            m_pImpl = std::make_shared<ReplicatorService::Impl>(
                    sirius::crypto::KeyPair::FromPrivate(std::move(sirius::crypto::PrivateKey{})),
                    m_networkIdentifier,
                    m_generationHash,
                    m_storageConfig,
                    m_transactionRangeHandler,
                    m_storageState
            );
        }
    }

    Key ReplicatorService::replicatorKey() const {
        if (m_pImpl)
            return m_pImpl->replicatorKey();

        return m_keyPair.publicKey();
    }

    void ReplicatorService::addDriveModification(const Key& driveKey,
                                                 const Hash256& downloadDataCdi,
                                                 const Hash256& modificationId,
                                                 const Key& owner,
                                                 uint64_t dataSize
    ) {
        if (m_pImpl)
            m_pImpl->addDriveModification(driveKey, downloadDataCdi, modificationId, owner, dataSize);
    }

    void ReplicatorService::removeDriveModification(const Key& driveKey, const Hash256& dataModificationId) {
        if (m_pImpl)
            m_pImpl->removeDriveModification(driveKey, dataModificationId);
    }

    void ReplicatorService::addDriveChannel(const Hash256& channelId,
                                            const Key& driveKey,
                                            size_t prepaidDownloadSize,
                                            const std::vector<Key>& consumers
    ) {
        if (m_pImpl)
            m_pImpl->addDriveChannel(channelId, driveKey, prepaidDownloadSize, consumers);
    }

    void ReplicatorService::closeDriveChannel(const Hash256& channelId) {
        if (m_pImpl)
            m_pImpl->closeDriveChannel(channelId);
    }

    void ReplicatorService::addDrive(const Key& driveKey, uint64_t driveSize, uint64_t usedSize, const utils::KeySet& replicators) {
        if (m_pImpl)
            m_pImpl->addDrive(driveKey, driveSize, usedSize, replicators);
    }

    bool ReplicatorService::driveExist(const Key& driveKey) {
        if (m_pImpl)
            m_pImpl->driveExist(driveKey);

        return false;
    }

    void ReplicatorService::closeDrive(const Key& driveKey, const Hash256& transactionHash) {
        if (m_pImpl)
            m_pImpl->closeDrive(driveKey, transactionHash);
    }

    void ReplicatorService::shutdown() {
        if (m_pImpl)
            m_pImpl->shutdown();
    }

    void ReplicatorService::terminate() {
        if (m_pImpl)
            m_pImpl->terminate();
    }

    // endregion
}}
