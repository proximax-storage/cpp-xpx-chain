/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ReplicatorService.h"

#include <utility>
#include "StoragePacketHandlers.h"
#include "TransactionSender.h"
#include "sdk/src/extensions/TransactionExtensions.h"
#include "catapult/utils/NetworkTime.h"
#include "catapult/extensions/ServiceState.h"
#include "catapult/extensions/ServiceLocator.h"
#include "drive/Replicator.h"

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

//					auto replicatorData = storageState.getReplicatorData(m_pReplicatorService->replicatorKey(), state.cache());
//					for (const auto& pair : replicatorData.Drives) {
//						m_pReplicatorService->addDrive(pair.first, pair.second);
//						const auto& driveModifications = replicatorData.DriveModifications[pair.first];
//						for (const auto& driveModification : driveModifications)
//							m_pReplicatorService->addDriveModification(
//                                    pair.first,
//                                    driveModification.first,
//                                    driveModification.second,
//                                    replicatorData.
//                            );
//					}
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
                StorageConfiguration&& storageConfig,
                handlers::TransactionRangeHandler transactionRangeHandler)
                : m_stopped(false)
                , m_keyPair(std::move(keyPair))
                , m_transactionSender(TransactionSender(
                        networkIdentifier,
                        generationHash,
                        std::move(transactionRangeHandler),
                        storageConfig
                ))
                , m_eventHandler(ReplicatorEventHandler(m_transactionSender))
                , m_pReplicator(sirius::drive::createDefaultReplicator(
                        keyPair,
                        Replicator_Host,
                        std::move(storageConfig.Port),
                        std::move(storageConfig.StorageDirectory),
                        std::move(storageConfig.SandboxDirectory),
                        storageConfig.UseTcpSocket,
                        m_eventHandler,
                        Service_Name
                ))
        {}

    public:
        Key replicatorKey() const {
            Key convertedKey;
            std::copy(m_keyPair.publicKey().begin(), m_keyPair.publicKey().end(), convertedKey.begin());

            return convertedKey;
        }

        void addDriveModification(const Key& driveKey, sirius::drive::ModifyRequest&& modifyRequest) {
            CATAPULT_LOG(debug) << "new modify request for " << driveKey << "drive: \n\t"
                                << modifyRequest;

            if (m_stopped)
                return;

            std::string error = m_pReplicator->modify((const sirius::Key&) driveKey, std::move(modifyRequest));
            if (!error.empty())
                CATAPULT_THROW_INVALID_ARGUMENT_1("drive modification finished with error: ", error);

            CATAPULT_LOG(debug) << "successfully sent the request to modify the drive";
        }

        void removeDriveModification(const Key& driveKey, const Hash256& transactionHash) {
            CATAPULT_LOG(debug) << "new modify request for " << driveKey << "drive: \n"
                                << "\ttransactions hash:" << transactionHash;

            if (m_stopped)
                return;

            std::string error = m_pReplicator->cancelModify(
                    (const sirius::Key&) driveKey,
                    (const sirius::Hash256&) transactionHash
            );

            if (!error.empty())
                CATAPULT_THROW_INVALID_ARGUMENT_1("drive cancel modification finished with error: ", error);

            CATAPULT_LOG(debug) << "successfully sent the request to cancel the drive modification";
        }

        void closeDrive(const Key& driveKey, const Hash256& transactionHash) {
            CATAPULT_LOG(debug) << "closing drive " << driveKey;

            if (m_stopped)
                return;

            auto error = m_pReplicator->removeDrive(
                    (const sirius::Key&) driveKey,
                    (const sirius::Hash256&) transactionHash
            );

            if (!error.empty())
                CATAPULT_THROW_INVALID_ARGUMENT_1("closeDrive finished with error: ", error);
        }

        void shutdown() {
            if (m_stopped)
                return;

            m_stopped = true;
        }

    private:
        bool m_stopped;
        sirius::crypto::KeyPair m_keyPair;
        TransactionSender m_transactionSender;
        ReplicatorEventHandler m_eventHandler;
        std::shared_ptr<sirius::drive::Replicator> m_pReplicator;
    };

    void ReplicatorService::start() {
        if (!m_pImpl) {
            sirius::crypto::PrivateKey convertedPrivateKey;
            std::copy(m_keyPair.privateKey().begin(), m_keyPair.privateKey().end(), convertedPrivateKey.begin());

            m_pImpl = std::make_shared<ReplicatorService::Impl>(
                    sirius::crypto::KeyPair::FromPrivate(std::move(convertedPrivateKey)),
                    m_networkIdentifier,
                    m_generationHash,
                    std::move(m_storageConfig),
                    m_transactionRangeHandler
            );
        }
    }

    Key ReplicatorService::replicatorKey() const {
        if (m_pImpl)
            return m_pImpl->replicatorKey();

        return m_keyPair.publicKey();
    }

    void ReplicatorService::shutdown() {
        if (m_pImpl)
            m_pImpl->shutdown();
    }

    void ReplicatorService::addDriveModification(
            const Key& driveKey,
            const Hash256& dataInfoHash,
            const Hash256& transactionHash,
            const Key& owner,
            uint64_t dataSize
    ) {
        auto modifyRequest = sirius::drive::ModifyRequest{
                (const sirius::utils::ByteArray<32, sirius::Hash256_tag>&) dataInfoHash,
                (const sirius::utils::ByteArray<32, sirius::Hash256_tag>&) transactionHash,
                dataSize,
                {},
                (const sirius::utils::ByteArray<32, sirius::Key_tag>&) owner
        };

        if (m_pImpl)
            m_pImpl->addDriveModification(driveKey, std::move(modifyRequest));
    }

    void ReplicatorService::removeDriveModification(const Key& driveKey, const Hash256& dataModificationId) {
        if (m_pImpl)
            m_pImpl->removeDriveModification(driveKey, dataModificationId);
    }

    void ReplicatorService::closeDrive(const Key& driveKey, const Hash256& transactionHash) {
        if (m_pImpl)
            m_pImpl->closeDrive(driveKey, transactionHash);
    }

    // endregion
}}
