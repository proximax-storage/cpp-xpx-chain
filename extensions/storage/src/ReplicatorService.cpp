/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma GCC diagnostic error "-Wmissing-field-initializers"

#include "ReplicatorService.h"
#include "TransactionSender.h"
#include "TransactionStatusHandler.h"
#include "catapult/extensions/ServiceLocator.h"
#include "catapult/extensions/ServiceState.h"
#include "catapult/io/BlockStorageCache.h"
#include "catapult/thread/MultiServicePool.h"
#include "drive/RpcReplicator.h"
#include <map>
#include <unordered_map>

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
				m_pReplicatorService->setServiceState(&state);
				auto pServiceGroup = state.pool().pushServiceGroup(Service_Name);
				pServiceGroup->pushService([pReplicatorService = m_pReplicatorService](const auto& pPool) {
					pReplicatorService->setThreadPool(pPool);
					return pReplicatorService;
				});

                auto& storageState = state.pluginManager().storageState();
				storageState.setReplicatorKey(m_pReplicatorService->replicatorKey());
                if (storageState.isReplicatorRegistered()) {
					m_pReplicatorService->post([](const auto& pReplicatorService) {
						CATAPULT_LOG(debug) << "starting replicator service";
						pReplicatorService->markRegistered();
						pReplicatorService->start();
					});
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
				, m_pTransactionStatusHandler(std::make_shared<TransactionStatusHandler>())
				, m_bootstrapReplicators(bootstrapReplicators)
				, m_initialized(false)
		{}

    public:
        void start(const StorageConfiguration& storageConfig, const Timestamp& timestamp, ReplicatorEventHandler& handler) {
			const auto& config = m_serviceState.config();
			if (storageConfig.UseRpcReplicator) {
				gHandleLostConnection = storageConfig.RpcHandleLostConnection;
				gDbgRpcChildCrash = storageConfig.RpcDbgChildCrash;
				m_pReplicator = sirius::drive::createRpcReplicator(
						storageConfig.RpcHost,
						std::stoi(storageConfig.RpcPort),
						reinterpret_cast<const sirius::crypto::KeyPair&>(m_keyPair), // TODO: pass private key string.
						storageConfig.Host,
						storageConfig.Port,
						storageConfig.StorageDirectory,
						resolveBootstrapAddresses(),
						storageConfig.UseTcpSocket,
						handler, // TODO: pass unique_ptr instead of ref.
						nullptr,
						Service_Name,
						storageConfig.LogOptions);
			}
			else {
				m_pReplicator = sirius::drive::createDefaultReplicator(
						reinterpret_cast<const sirius::crypto::KeyPair&>(m_keyPair), // TODO: pass private key string.
						storageConfig.Host,
						storageConfig.Port,
						storageConfig.StorageDirectory,
						resolveBootstrapAddresses(),
						storageConfig.UseTcpSocket,
						handler, // TODO: pass unique_ptr instead of ref.
						nullptr,
						Service_Name,
						storageConfig.LogOptions);
			}
			m_pReplicator->setServiceAddress(storageConfig.RpcServicesServerHost + ":" + storageConfig.RpcServicesServerPort);
			m_pReplicator->enableSupercontractServer();
			m_pReplicator->enableMessengerServer();
			m_pReplicator->start();

            auto drives = m_storageState.getDrives(timestamp);
            for (const auto& pDrive: drives)
                addDrive(pDrive);

			m_pReplicator->asyncInitializationFinished();

			m_initialized = true;
        }

        void addDriveModification(
                const Key& driveKey,
                const Hash256& downloadDataCdi,
                const Hash256& modificationId,
                const Key& owner,
                uint64_t dataSizeMegabytes,
				const utils::KeySet& replicators) {
            CATAPULT_LOG(debug) << "new modify request " << modificationId << " for drive " << driveKey;
            m_pReplicator->asyncModify(
				driveKey.array(),
				std::make_unique<sirius::drive::ModificationRequest>(sirius::drive::ModificationRequest{
					downloadDataCdi.array(),
					modificationId.array(), utils::FileSize::FromMegabytes(dataSizeMegabytes).bytes(),
					castReplicatorKeys<sirius::Key>(replicators)}));
        }

		void startStream(
				const std::shared_ptr<state::Drive>& pDrive,
				const Hash256& streamId,
				const std::string& folder,
				uint64_t expectedSizeMegabytes,
				bool updateDrive = true) {
			CATAPULT_LOG(debug) << "new stream request " << streamId << " for drive " << pDrive->Id;
			m_pReplicator->asyncStartStream(pDrive->Id.array(),
				std::make_unique<sirius::drive::StreamRequest>(sirius::drive::StreamRequest{ streamId.array(),
					pDrive->Owner.array(),
					folder,
					utils::FileSize::FromMegabytes(expectedSizeMegabytes).bytes(),
					castReplicatorKeys<sirius::Key>(pDrive->Replicators) }));

			if (updateDrive)
				m_drives[pDrive->Id] = pDrive;
		}

		void streamPaymentPublished(const std::shared_ptr<state::Drive>& pDrive, const Hash256& streamId) {
        	CATAPULT_LOG(debug) << "stream payment published " << streamId << " for drive " << pDrive->Id;
			auto modificationIt = std::find_if(pDrive->DataModifications.begin(), pDrive->DataModifications.end(), [&streamId](const auto& modification) {
				return modification.Id == streamId;
			});

			if (modificationIt == pDrive->DataModifications.end()) {
				CATAPULT_LOG(error) << "stream for payment increasing not found " << streamId;
				return;
			}

			m_pReplicator->asyncIncreaseStream(pDrive->Id.array(),
				std::make_unique<sirius::drive::StreamIncreaseRequest>(sirius::drive::StreamIncreaseRequest{
					streamId.array(),
					modificationIt->ExpectedUploadSize,
				}
 			));
		}

		void finishStream(
				const std::shared_ptr<state::Drive>& pDrive,
				const Hash256& streamId,
				const Hash256& finishDownloadDataCdi,
				uint64_t actualSizeMegabytes,
				bool updateDrive = true) {
        	CATAPULT_LOG(debug) << "finish stream request " << streamId << " for drive " << pDrive->Id;
			m_pReplicator->asyncFinishStreamTxPublished(
				pDrive->Id.array(),
				std::make_unique<sirius::drive::StreamFinishRequest>(sirius::drive::StreamFinishRequest {
					streamId.array(),
					finishDownloadDataCdi.array(),
					utils::FileSize::FromMegabytes(actualSizeMegabytes).bytes(),
				}
 			));

			if (updateDrive)
				m_drives[pDrive->Id] = pDrive;
		}

		void removeDriveModification(const std::shared_ptr<state::Drive>& pDrive, const Hash256& transactionHash) {
            CATAPULT_LOG(debug) << "remove modify request " << transactionHash.data() << " for drive " << pDrive->Id;
			m_drives[pDrive->Id] = pDrive;
            m_pReplicator->asyncCancelModify(pDrive->Id.array(), transactionHash.array());
        }

        void addDownloadChannel(const std::shared_ptr<state::DownloadChannel>& pChannel) {
        	CATAPULT_LOG(debug) << "add download channel " << pChannel->Id;
			auto driveIter = m_drives.find(pChannel->DriveKey);
			if (driveIter == m_drives.end() || driveIter->second->DownloadChannels.find(pChannel->Id) != driveIter->second->DownloadChannels.end()) {
				CATAPULT_LOG(warning) << "download channel already added " << pChannel->Id;
				return;
			}

			m_pReplicator->asyncAddDownloadChannelInfo(
					pChannel->DriveKey.array(),
					std::make_unique<sirius::drive::DownloadRequest>(sirius::drive::DownloadRequest{
						pChannel->Id.array(),
						utils::FileSize::FromMegabytes(pChannel->DownloadSizeMegabytes).bytes(),
						castReplicatorKeys<sirius::Key>(pChannel->Replicators),
						castReplicatorKeys<sirius::Key>(pChannel->Consumers)}));

			if (pChannel->ApprovalTrigger)
				m_pReplicator->asyncInitiateDownloadApprovalTransactionInfo(pChannel->ApprovalTrigger->array(), pChannel->Id.array());

			driveIter->second->DownloadChannels[pChannel->Id] = pChannel;
        }

		void removeDownloadChannel(const std::shared_ptr<state::Drive>& pDrive, const std::shared_ptr<state::DownloadChannel>& pChannel) {
			pDrive->DownloadChannels.erase(pChannel->Id);
			m_pReplicator->asyncRemoveDownloadChannelInfo(pChannel->Id.array());
		}

        void increaseDownloadChannelSize(const std::shared_ptr<state::DownloadChannel>& pChannel) {
            CATAPULT_LOG(debug) << "updating download channel " << pChannel->Id;

			m_pReplicator->asyncIncreaseDownloadChannelSize(pChannel->Id.array(), utils::FileSize::FromMegabytes(pChannel->DownloadSizeMegabytes).bytes());
        }

        void initiateDownloadApproval(const std::vector<std::shared_ptr<state::DownloadChannel>>& channels) {
			for (const auto& pChannel : channels) {
				auto driveIter = m_drives.find(pChannel->DriveKey);
				bool channelAdded = (driveIter != m_drives.end() && driveIter->second->DownloadChannels.find(pChannel->Id) != driveIter->second->DownloadChannels.end());
				bool isParticipant = (pChannel->Replicators.find(m_keyPair.publicKey()) != pChannel->Replicators.cend());
				if (pChannel->ApprovalTrigger && channelAdded && isParticipant) {
					CATAPULT_LOG(debug) << "initiate download approval " << pChannel->Id;
					m_pReplicator->asyncInitiateDownloadApprovalTransactionInfo(pChannel->ApprovalTrigger->array(), pChannel->Id.array());
				}
			}
        }

        void addDrive(const std::shared_ptr<state::Drive>& pDrive) {
			CATAPULT_LOG(debug) << "add drive " << pDrive->Id;
			if (m_drives.find(pDrive->Id) != m_drives.end()) {
				CATAPULT_LOG(warning) << "The drive has already been added";
				return;
			}

			std::vector<sirius::drive::CompletedModification> storageCompletedModifications;
			for (const auto& modification: pDrive->CompletedModifications) {
				sirius::drive::CompletedModification completedModification;
				completedModification.m_modificationId = modification.ModificationId.array();
				if (modification.Status == state::DataModificationApprovalState::Cancelled) {
					completedModification.m_completedModificationStatus = sirius::drive::CompletedModification::CompletedModificationStatus::CANCELLED;
				}
				else {
					completedModification.m_completedModificationStatus = sirius::drive::CompletedModification::CompletedModificationStatus::APPROVED;
				}
				storageCompletedModifications.push_back(completedModification);
			}

			sirius::drive::AddDriveRequest request {
				utils::FileSize::FromMegabytes(pDrive->Size).bytes(),
				pDrive->DownloadWorkBytes,
				storageCompletedModifications,
				castReplicatorKeys<sirius::Key>(pDrive->Replicators),
				pDrive->Owner.array(),
				castReplicatorKeys<sirius::Key>(pDrive->DonatorShard),
				castReplicatorKeys<sirius::Key>(pDrive->RecipientShard)
			};

			m_pReplicator->asyncAddDrive(pDrive->Id.array(), std::make_unique<sirius::drive::AddDriveRequest>(request));

			if (pDrive->LastApprovedDataModificationPtr) {
				m_pReplicator->asyncApprovalTransactionHasBeenPublished(
					std::make_unique<sirius::drive::PublishedModificationApprovalTransactionInfo>(
						sirius::drive::PublishedModificationApprovalTransactionInfo{
						pDrive->LastApprovedDataModificationPtr->DriveKey.array(),
						pDrive->LastApprovedDataModificationPtr->Id.array(),
						pDrive->RootHash.array(),
						castReplicatorKeys<std::array<uint8_t, 32>>(pDrive->LastApprovedDataModificationPtr->Signers)}));
			}

			if (pDrive->ActiveVerificationPtr && !pDrive->ActiveVerificationPtr->Expired)
				startVerification(pDrive->Id, *pDrive->ActiveVerificationPtr);

			for (const auto& dataModification: pDrive->DataModifications) {
				if (dataModification.IsStream) {
					startStream(
						pDrive,
						dataModification.Id,
						dataModification.FolderName,
						dataModification.ExpectedUploadSize,
						false);

					if (dataModification.ReadyForApproval) {
						finishStream(
							pDrive,
							dataModification.Id,
							dataModification.DownloadDataCdi,
							dataModification.ActualUploadSize,
							false);
					}
				}
				else {
					addDriveModification(
						pDrive->Id,
						dataModification.DownloadDataCdi,
						dataModification.Id,
						pDrive->Owner,
						dataModification.ExpectedUploadSize,
						pDrive->Replicators);
				}
			}

			for (const auto& [_, pChannel] : pDrive->DownloadChannels) {
				if (pChannel->Replicators.find(m_keyPair.publicKey()) != pChannel->Replicators.cend())
					addDownloadChannel(pChannel);
			}

			m_drives[pDrive->Id] = pDrive;
        }

        void updateDrive(const std::shared_ptr<state::Drive>& pUpdatedDrive) {
			CATAPULT_LOG(debug) << "update drive " << pUpdatedDrive->Id;
			auto iter = m_drives.find(pUpdatedDrive->Id);
			if (iter == m_drives.end()) {
				CATAPULT_LOG(warning) << "drive not found " << pUpdatedDrive->Id;
				return;
			}

			auto& pDrive = iter->second;
			m_pReplicator->asyncSetReplicators(pDrive->Id.array(), std::make_unique<sirius::drive::ReplicatorList>(castReplicatorKeys<sirius::Key>(pUpdatedDrive->Replicators)));
			m_pReplicator->asyncSetShardDonator(pDrive->Id.array(), std::make_unique<sirius::drive::ReplicatorList>(castReplicatorKeys<sirius::Key>(pUpdatedDrive->DonatorShard)));
			m_pReplicator->asyncSetShardRecipient(pDrive->Id.array(), std::make_unique<sirius::drive::ReplicatorList>(castReplicatorKeys<sirius::Key>(pUpdatedDrive->RecipientShard)));

			for (const auto& [_, pChannel] : pUpdatedDrive->DownloadChannels) {
				bool channelAdded = (pDrive->DownloadChannels.find(pChannel->Id) != pDrive->DownloadChannels.end());
				if (pChannel->Replicators.find(m_keyPair.publicKey()) != pChannel->Replicators.cend()) {
					if (channelAdded) {
						updateDownloadChannelReplicators(pChannel);
					} else {
						// this replicator is now assigned to the channel
						addDownloadChannel(pChannel);
					}
				} else if (channelAdded) {
					removeDownloadChannel(pDrive, pChannel);
				}
			}

			m_drives[pDrive->Id] = pUpdatedDrive;
		}

		void removeDrive(const Key& driveKey) {
        	CATAPULT_LOG(debug) << "remove drive " << driveKey;
			auto iter = m_drives.find(driveKey);
			if (iter == m_drives.end()) {
				CATAPULT_LOG(warning) << "drive not found " << driveKey;
				return;
			}

			auto pDrive = iter->second;
			for (const auto& [_, channel] : pDrive->DownloadChannels)
				removeDownloadChannel(pDrive, channel);
			m_pReplicator->asyncRemoveDrive(driveKey.array());
			m_drives.erase(driveKey);
		}

        void closeDrive(const Key& driveKey, const Hash256& eventHash) {
			auto iter = m_drives.find(driveKey);
			if (iter == m_drives.end())
				return;

			CATAPULT_LOG(debug) << "closing drive " << driveKey;
			m_pReplicator->asyncCloseDrive(driveKey.array(), eventHash.array());
			m_drives.erase(iter);
        }

        bool driveAdded(const Key& driveKey) {
			return m_drives.find(driveKey) != m_drives.cend();
		}

		bool channelAdded(const std::shared_ptr<state::DownloadChannel>& pChannel) {
			auto driveIter = m_drives.find(pChannel->DriveKey);
			return (driveIter != m_drives.end() && driveIter->second->DownloadChannels.find(pChannel->Id) != driveIter->second->DownloadChannels.end());
		}

		void processVerifications(const std::unordered_map<Key, std::shared_ptr<state::DriveVerification>, utils::ArrayHasher<Key>>& verifications) {
			for (const auto& [driveKey, pVerification] : verifications) {
				m_pReplicator->asyncCancelDriveVerification(driveKey.array());
				startVerification(driveKey, *pVerification);
			}
        }

		void updateDownloadChannelReplicators(const std::shared_ptr<state::DownloadChannel>& pChannel) {
        	CATAPULT_LOG(debug) << "update channel replicators " << pChannel->Id;
			m_pReplicator->asyncSetChanelShard(std::make_unique<sirius::Hash256>(
				pChannel->Id.array()), std::make_unique<sirius::drive::ReplicatorList>(castReplicatorKeys<sirius::Key>(pChannel->Replicators)));
		}

		void updateDrives(const std::vector<std::shared_ptr<state::Drive>>& drives) {
        	for (const auto& pDrive: drives) {
				if (pDrive->Replicators.find(m_keyPair.publicKey()) == pDrive->Replicators.cend())
					continue;

        		if (m_drives.find(pDrive->Id) == m_drives.end()) {
        			addDrive(pDrive);
        		} else {
        			updateDrive(pDrive);
        		}
        	}
		}

		void dataModificationApprovalPublished(
				const std::shared_ptr<state::Drive>& pDrive,
                const Hash256& modificationId,
                const Hash256& rootHash,
                const std::vector<Key>& replicators) {
			m_drives[pDrive->Id] = pDrive;
            m_pReplicator->asyncApprovalTransactionHasBeenPublished(std::make_unique<sirius::drive::PublishedModificationApprovalTransactionInfo>(
				sirius::drive::PublishedModificationApprovalTransactionInfo {
					pDrive->Id.array(), modificationId.array(), rootHash.array(), castReplicatorKeys<std::array<uint8_t, 32>>(replicators)
				}
 			));
		}

        void dataModificationSingleApprovalPublished(const std::shared_ptr<state::Drive>& pDrive, const Hash256& modificationId) {
			m_drives[pDrive->Id] = pDrive;
            m_pReplicator->asyncSingleApprovalTransactionHasBeenPublished(
				std::make_unique<sirius::drive::PublishedModificationSingleApprovalTransactionInfo>(
					sirius::drive::PublishedModificationSingleApprovalTransactionInfo{ pDrive->Id.array(), modificationId.array() }));
        }

        void downloadApprovalPublished(const std::shared_ptr<state::DownloadChannel>& pChannel, bool downloadChannelClosed) {
			m_drives.at(pChannel->DriveKey)->DownloadChannels.at(pChannel->Id)->ApprovalTrigger.reset();
			m_pReplicator->asyncDownloadApprovalTransactionHasBeenPublished(pChannel->ApprovalTrigger->array(), pChannel->Id.array(), downloadChannelClosed);
        }

        void endDriveVerificationPublished(const Key& driveKey, const Hash256& verificationTrigger) {
        	m_pReplicator->asyncVerifyApprovalTransactionHasBeenPublished({verificationTrigger.array(), driveKey.array()});
		}

        void notifyTransactionStatus(const Hash256& hash, uint32_t status) {
            m_pTransactionStatusHandler->handle(hash, status);
        }

        void stop() {
			m_pReplicator->shutdownReplicator();
			m_pReplicator.reset();
        }

        bool isAlive() const {
        	return !m_pReplicator->isConnectionLost();
        }

        bool initialized() const {
        	return m_initialized;
        }

		std::shared_ptr<TransactionStatusHandler> transactionStatusHandler() {
			return m_pTransactionStatusHandler;
		}

		std::shared_ptr<sirius::drive::Replicator> replicator() {
			return m_pReplicator;
		}

		std::shared_ptr<state::Drive> getDrive(const Key& driveKey) {
			auto iter = m_drives.find(driveKey);
			if (iter != m_drives.cend())
				return iter->second;

			return nullptr;
		}

		std::shared_ptr<state::DownloadChannel> getDownloadChannel(const Hash256& channelId) {
			for (const auto& [_, pDrive] : m_drives) {
				auto iter = pDrive->DownloadChannels.find(channelId);
				if (iter != pDrive->DownloadChannels.cend())
					return iter->second;
			}

			return nullptr;
		}

    private:

		void startVerification( const Key& driveKey, const state::DriveVerification& verification ) {
			std::set<sirius::Key> flattenShards;
			for (const auto& shard: verification.Shards) {
				for (const auto& key: shard)
					flattenShards.insert(key.array());
			}

			bool foundShard = false;
			for (uint16_t i = 0u; i < verification.Shards.size() && !foundShard; ++i) {
				const auto& shard = verification.Shards[i];
				if (shard.find(m_keyPair.publicKey()) != shard.end()) {
					foundShard = true;
					m_pReplicator->asyncStartDriveVerification(
						driveKey.array(),
						std::make_unique<sirius::drive::VerificationRequest>(
							sirius::drive::VerificationRequest {
								verification.VerificationTrigger.array(),
								i,
								verification.ModificationId.array(),
								castReplicatorKeys<sirius::Key>(shard),
								verification.Duration,
								flattenShards
							}
 					));
				}
			}
		}

		std::vector<sirius::drive::ReplicatorInfo> resolveBootstrapAddresses() {
			boost::asio::io_context ctx;
			boost::asio::ip::tcp::resolver resolver(ctx);

			std::vector<sirius::drive::ReplicatorInfo> bootstrapReplicators;
			bootstrapReplicators.reserve(m_bootstrapReplicators.size());
			for (const auto& node : m_bootstrapReplicators) {
				const auto host = node.endpoint().Host;
				if (host.empty()) {
					CATAPULT_LOG(warning) << "skipping empty host for " << node.identityKey();
					continue;
				}

				const auto port = std::to_string(node.endpoint().Port);

				boost::system::error_code ec;
				auto result = resolver.resolve(host, port, ec);
				if (ec) {
					CATAPULT_LOG(warning) << "endpoint not resolved " << host << ":" << port << " " << ec.message();
				} else {
					auto endpoint = result.begin()->endpoint();
					auto udpEndpoint = boost::asio::ip::udp::endpoint{ endpoint.address(), endpoint.port() };
					auto publicKey = node.identityKey().array();
					bootstrapReplicators.emplace_back(sirius::drive::ReplicatorInfo{ udpEndpoint, publicKey });
				}
			}

			if (bootstrapReplicators.empty())
				CATAPULT_THROW_RUNTIME_ERROR("bootstrap replicators cannot be empty")

			return bootstrapReplicators;
		}

		template<class T, typename Container>
		std::vector<T> castReplicatorKeys(const Container& keys) {
            std::vector<T> replicators;
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
        std::shared_ptr<TransactionStatusHandler> m_pTransactionStatusHandler;
		const std::vector<ionet::Node>& m_bootstrapReplicators;

		std::unordered_map<Key, std::shared_ptr<state::Drive>, utils::ArrayHasher<Key>> m_drives;
		bool m_initialized;
    };

    // endregion

    // region - replicator service

    ReplicatorService::ReplicatorService(StorageConfiguration&& storageConfig, std::vector<ionet::Node>&& bootstrapReplicators)
		: m_keyPair(crypto::KeyPair::FromString(storageConfig.Key))
		, m_storageConfig(std::move(storageConfig))
		, m_bootstrapReplicators(std::move(bootstrapReplicators))
		, m_registered(false)
	{}

	ReplicatorService::~ReplicatorService() {
		stop();
	}

	void ReplicatorService::post(const consumer<const std::shared_ptr<ReplicatorService>&>& handler) {
		boost::asio::post(*m_pStrand, [pReplicatorServiceWeak = weak_from_this(), handler]() {
			auto pReplicatorService = pReplicatorServiceWeak.lock();
			if (!pReplicatorService)
				return;

			handler(pReplicatorService);
		});
	}

	bool ReplicatorService::registered() {
		return m_registered;
	}

	void ReplicatorService::markRegistered() {
		m_registered = true;
	}

	bool ReplicatorService::isAlive() {
		return m_pImpl && m_pImpl->isAlive();
	}

	bool ReplicatorService::initialized() {
		return m_pImpl && m_pImpl->initialized();
	}

    void ReplicatorService::start() {
        if (m_pImpl)
			CATAPULT_THROW_RUNTIME_ERROR("replicator service already started");

        m_pImpl = std::make_unique<ReplicatorService::Impl>(
			m_keyPair,
			*m_pServiceState,
			m_pServiceState->pluginManager().storageState(),
			m_bootstrapReplicators);
        m_pImpl->start(m_storageConfig, m_pServiceState->timeSupplier()(), *this);
    }

    void ReplicatorService::stop() {
        if (m_pImpl) {
            m_pImpl->stop();
            m_pImpl.reset();
        }
    }

    void ReplicatorService::shutdown() {
        stop();
    }

	void ReplicatorService::setServiceState(extensions::ServiceState* pServiceState) {
		m_pServiceState = pServiceState;
		m_pTransactionSender = std::make_unique<TransactionSender>(
			m_keyPair,
			m_pServiceState->config().Immutable,
			m_storageConfig,
			m_pServiceState->hooks().transactionRangeConsumerFactory()(disruptor::InputSource::Local));
	}

	void ReplicatorService::setThreadPool(const std::shared_ptr<thread::IoThreadPool>& pPool) {
		m_pPool = pPool;
		m_pStrand = std::make_unique<boost::asio::io_context::strand>(m_pPool->ioContext());
	}

    const Key& ReplicatorService::replicatorKey() const {
        return m_keyPair.publicKey();
    }

	std::shared_ptr<TransactionStatusHandler> ReplicatorService::transactionStatusHandler() {
		if (m_pImpl)
			return m_pImpl->transactionStatusHandler();

		return nullptr;
	}

	std::shared_ptr<sirius::drive::Replicator> ReplicatorService::replicator() {
		if (m_pImpl)
			return m_pImpl->replicator();

		return nullptr;
	}

    void ReplicatorService::addDriveModification(
            const Key& driveKey,
            const Hash256& downloadDataCdi,
            const Hash256& modificationId,
            const Key& owner,
            uint64_t dataSizeMegabytes,
			const utils::KeySet& replicators) {
        if (m_pImpl)
        	m_pImpl->addDriveModification(driveKey, downloadDataCdi, modificationId, owner, dataSizeMegabytes, replicators);
    }

	void ReplicatorService::startStream(
			const std::shared_ptr<state::Drive>& pDrive,
			const Hash256& streamId,
			const std::string& folder,
			uint64_t expectedSizeMegabytes) {
		if (m_pImpl)
			m_pImpl->startStream(pDrive, streamId, folder, expectedSizeMegabytes);
	}

	void ReplicatorService::streamPaymentPublished(const std::shared_ptr<state::Drive>& pDrive, const Hash256& streamId) {
		if (m_pImpl)
			m_pImpl->streamPaymentPublished(pDrive, streamId);
	}

	void ReplicatorService::finishStream(
			const std::shared_ptr<state::Drive>& pDrive,
			const Hash256& streamId,
			const Hash256& finishDownloadDataCdi,
			uint64_t actualSizeMegabytes) {
    	if (m_pImpl)
    		m_pImpl->finishStream(pDrive, streamId, finishDownloadDataCdi, actualSizeMegabytes);
	}

	void ReplicatorService::removeDriveModification(const std::shared_ptr<state::Drive>& pDrive, const Hash256& dataModificationId) {
        if (m_pImpl)
            m_pImpl->removeDriveModification(pDrive, dataModificationId);
    }

    void ReplicatorService::addDownloadChannel(const std::shared_ptr<state::DownloadChannel>& pChannel) {
        if (m_pImpl)
            m_pImpl->addDownloadChannel(pChannel);
    }

    void ReplicatorService::increaseDownloadChannelSize(const std::shared_ptr<state::DownloadChannel>& pChannel) {
        if (m_pImpl)
            m_pImpl->increaseDownloadChannelSize(pChannel);
    }

    void ReplicatorService::initiateDownloadApproval(const std::vector<std::shared_ptr<state::DownloadChannel>>& channels) {
        if (m_pImpl)
			m_pImpl->initiateDownloadApproval(channels);
    }

    void ReplicatorService::endDriveVerificationPublished(const Key& driveKey, const Hash256& verificationTrigger) {
    	if (m_pImpl)
    		m_pImpl->endDriveVerificationPublished(driveKey, verificationTrigger);
    }

    void ReplicatorService::addDrive(const std::shared_ptr<state::Drive>& pDrive) {
        if (m_pImpl)
            m_pImpl->addDrive(pDrive);
    }

    void ReplicatorService::updateDrive(const std::shared_ptr<state::Drive>& pUpdatedDrive) {
        if (m_pImpl)
            m_pImpl->updateDrive(pUpdatedDrive);
    }

	void ReplicatorService::removeDrive(const Key& driveKey) {
    	if (m_pImpl)
    		m_pImpl->removeDrive(driveKey);
	}

    void ReplicatorService::closeDrive(const Key& driveKey, const Hash256& transactionHash) {
        if (m_pImpl)
            m_pImpl->closeDrive(driveKey, transactionHash);
    }

    bool ReplicatorService::driveAdded(const Key& driveKey) {
    	if (m_pImpl)
    		return m_pImpl->driveAdded(driveKey);

		return false;
	}

	bool ReplicatorService::channelAdded(const std::shared_ptr<state::DownloadChannel>& pChannel) {
    	if (m_pImpl)
    		return m_pImpl->channelAdded(pChannel);

    	return false;
    }

    void ReplicatorService::updateDrives(const std::vector<std::shared_ptr<state::Drive>>& drives) {
    	if (m_pImpl)
    		m_pImpl->updateDrives(drives);
	}

	void ReplicatorService::processVerifications(const std::unordered_map<Key, std::shared_ptr<state::DriveVerification>, utils::ArrayHasher<Key>>& verifications) {
        if (m_pImpl)
        	m_pImpl->processVerifications(verifications);
    }

    void ReplicatorService::notifyTransactionStatus(const Hash256& hash, uint32_t status) {
        if (m_pImpl)
            m_pImpl->notifyTransactionStatus(hash, status);
    }

    void ReplicatorService::dataModificationApprovalPublished(
			const std::shared_ptr<state::Drive>& pDrive,
            const Hash256& modificationId,
            const Hash256& rootHash,
            const std::vector<Key>& replicators) {
        if (m_pImpl)
            m_pImpl->dataModificationApprovalPublished(pDrive, modificationId, rootHash, replicators);
    }

    void ReplicatorService::dataModificationSingleApprovalPublished(const std::shared_ptr<state::Drive>& pDrive, const Hash256& modificationId) {
        if (m_pImpl)
            m_pImpl->dataModificationSingleApprovalPublished(pDrive, modificationId);
    }

    void ReplicatorService::downloadApprovalPublished(const std::shared_ptr<state::DownloadChannel>& pChannel, bool downloadChannelClosed) {
        if (m_pImpl)
            m_pImpl->downloadApprovalPublished(pChannel, downloadChannelClosed);
    }

	std::shared_ptr<state::Drive> ReplicatorService::getDrive(const Key& driveKey) {
		if (m_pImpl)
			return m_pImpl->getDrive(driveKey);

		return nullptr;
	}

	std::shared_ptr<state::DownloadChannel> ReplicatorService::getDownloadChannel(const Hash256& channelId) {
		if (m_pImpl)
			return m_pImpl->getDownloadChannel(channelId);

		return nullptr;
	}

    // endregion
}}
