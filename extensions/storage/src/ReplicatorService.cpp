/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma GCC diagnostic error "-Wmissing-field-initializers"

#include "ReplicatorService.h"
#include "ReplicatorEventHandler.h"
#include "TransactionSender.h"
#include "TransactionStatusHandler.h"
#include "catapult/extensions/ServiceLocator.h"
#include "catapult/extensions/ServiceState.h"
#include "catapult/io/BlockStorageCache.h"

#include <map>

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
                addDrive(drive.Id);
            }

            auto channels = m_storageState.getDownloadChannels(m_keyPair.publicKey());
            for (const auto& channel: channels) {
                addDownloadChannel(
					channel.Id,
					channel.DriveKey,
					channel.DownloadSizeMegabytes,
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
                uint64_t dataSizeMegabytes) {
            CATAPULT_LOG(debug) << "new modify request " << modificationId << " for drive " << driveKey;

            auto replicators = castReplicatorKeys<sirius::Key>(m_storageState.getDriveReplicators(driveKey));
            m_pReplicator->asyncModify(
				driveKey.array(),
				sirius::drive::ModificationRequest{
					downloadDataCdi.array(),
					modificationId.array(), utils::FileSize::FromMegabytes(dataSizeMegabytes).bytes(),
					replicators});
        }

		void removeDriveModification(const Key& driveKey, const Hash256& transactionHash) {
            CATAPULT_LOG(debug) << "remove modify request " << transactionHash.data() << " for drive " << driveKey;

            m_pReplicator->asyncCancelModify(driveKey.array(), transactionHash.array());
        }

        void addDownloadChannel(
                const Hash256& channelId,
                const Key& driveKey,
                size_t prepaidDownloadSizeMegabytes,
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
					channelId.array(), utils::FileSize::FromMegabytes(prepaidDownloadSizeMegabytes).bytes(),
					castedReplicators,
					castedConsumers});
        }

        void addDownloadChannel(const Hash256& channelId) {
            auto pChannel = m_storageState.getDownloadChannel(m_keyPair.publicKey(), channelId);
			if (!!pChannel) {
				addDownloadChannel(
					channelId,
					pChannel->DriveKey,
					pChannel->DownloadSizeMegabytes,
					pChannel->Consumers,
					pChannel->Replicators);
			}
        }

        void increaseDownloadChannelSize(const Hash256& channelId) {
            CATAPULT_LOG(debug) << "updating download channel " << channelId.data();

            auto pChannel = m_storageState.getDownloadChannel(m_keyPair.publicKey(), channelId);
			if (!!pChannel) {
				addDownloadChannel(
					channelId,
					pChannel->DriveKey,
					pChannel->DownloadSizeMegabytes,
					pChannel->Consumers,
					pChannel->Replicators);
			}
        }

        void initiateDownloadApproval(const Hash256& channelId, const Hash256& eventHash) {
        	CATAPULT_LOG(debug) << "initiate download approval" << channelId.data();

            auto pChannel = m_storageState.getDownloadChannel(m_keyPair.publicKey(), channelId);
			if (!!pChannel) {
				m_pReplicator->asyncInitiateDownloadApprovalTransactionInfo(eventHash.array(), channelId.array());
			}
        }

        void addDrive(const Key& driveKey) {
			CATAPULT_LOG(debug) << "add drive" << driveKey;

			if (  m_alreadyAddedDrives.find(driveKey) != m_alreadyAddedDrives.end() )
			{
				CATAPULT_LOG( error ) << "The drive is already added";
				return;
			}

			m_alreadyAddedDrives[driveKey] = m_storageState.getChainHeight();

            auto drive = m_storageState.getDrive(driveKey);
            std::vector<sirius::Key> replicators = castReplicatorKeys<sirius::Key>({drive.Replicators.begin(), drive.Replicators.end()});
            auto donatorShard = castReplicatorKeys<sirius::Key>(m_storageState.getDonatorShard(driveKey, m_keyPair.publicKey()));
            auto recipientShard = castReplicatorKeys<sirius::Key>(m_storageState.getRecipientShard(driveKey, m_keyPair.publicKey()));
            auto downloadWorkBytes = m_storageState.getDownloadWorkBytes(m_keyPair.publicKey(), driveKey);

			sirius::drive::AddDriveRequest request{drive.Size, downloadWorkBytes, replicators, m_storageState.getDrive(driveKey).Owner.array(), donatorShard, recipientShard};
			m_pReplicator->asyncAddDrive(driveKey.array(), request);

			auto pLastApprovedModification = m_storageState.getLastApprovedDataModification(drive.Id);
			if (!!pLastApprovedModification) {
				m_pReplicator->asyncApprovalTransactionHasBeenPublished(sirius::drive::PublishedModificationApprovalTransactionInfo{
					pLastApprovedModification->DriveKey.array(),
					pLastApprovedModification->Id.array(),
					pLastApprovedModification->DownloadDataCdi.array(),
					castReplicatorKeys<std::array<uint8_t, 32>>(pLastApprovedModification->Signers)});
			}

			auto pActiveVerification = m_storageState.getActiveVerification(drive.Id);
			if (!!pActiveVerification) {
				sirius::Hash256 verificationTrigger(pActiveVerification->VerificationTrigger.array());
				sirius::drive::InfoHash rootHash(pActiveVerification->RootHash.array());

				bool foundShard = false;
				for (uint32_t i = 0u; i < pActiveVerification->Shards.size() && !foundShard; ++i) {
					const auto& shard = pActiveVerification->Shards[i];

					if (std::find(shard.begin(), shard.end(), m_keyPair.publicKey()) != shard.end()) {
						foundShard = true;
						m_pReplicator->asyncStartDriveVerification(
								drive.Id.array(),
								sirius::drive::VerificationRequest {
									verificationTrigger, i, rootHash, castReplicatorKeys<sirius::Key>(shard) });
					}
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

		void removeDrive(const Key& driveKey)
		{
        	CATAPULT_LOG(debug) << "maybe remove drive " << driveKey;

			m_pReplicator->asyncRemoveDrive(driveKey.array());
			m_alreadyAddedDrives.erase(driveKey);
		}

        bool isAssignedToDrive(const Key& driveKey) {
			return m_storageState.isReplicatorAssignedToDrive(m_keyPair.publicKey(), driveKey);
        }

        bool isAssignedToChannel(const Hash256& channelId) {
			return m_storageState.isReplicatorAssignedToChannel(m_keyPair.publicKey(), channelId);
		}

        void closeDrive(const Key& driveKey, const Hash256& transactionHash) {
			CATAPULT_LOG(debug) << "closing drive " << driveKey;

			m_pReplicator->asyncCloseDrive(
					driveKey.array(),
					transactionHash.array());
			m_alreadyAddedDrives.erase(driveKey);
        }

        std::optional<Height> driveAddedAt(const Key& driveKey) {
        	if (auto it = m_alreadyAddedDrives.find(driveKey); it != m_alreadyAddedDrives.end()) {
				return it->second;
			}
			return {};
		}

		std::optional<Height> channelAddedAt(const Hash256& channelId) {
        	if (auto it = m_alreadyAddedChannels.find(channelId); it != m_alreadyAddedChannels.end()) {
        		return it->second;
        	}
        	return {};
		}

		void exploreNewDrives() {
        	auto drives = m_storageState.getReplicatorDriveKeys(m_keyPair.publicKey());
			for (const auto& drive: drives) {
				if (m_alreadyAddedDrives.find(drive) == m_alreadyAddedDrives.end()) {
					addDrive(drive);
				}
			}
		}

        void processVerifications(const Hash256& blockHash) {
			auto drives = m_storageState.getReplicatorDriveKeys(m_keyPair.publicKey());
			auto height = m_storageState.getChainHeight();
			for (const auto& driveKey: drives) {
				if (auto it = m_alreadyAddedDrives.find(driveKey); it != m_alreadyAddedDrives.end()) {

					if (it->second == height)
					{
						continue;
					}

					auto verification = m_storageState.getActiveVerification(driveKey);

					if (!verification) {
						continue;
					}

					sirius::Hash256 verificationTrigger = verification->VerificationTrigger.array();
					if (verification->Expired) {
						m_pReplicator->asyncCancelDriveVerification(driveKey.array(),verificationTrigger);
					}
					else if (verification->VerificationTrigger == blockHash) {
						m_pReplicator->asyncStartDriveVerification(driveKey.array(), verificationTrigger);
					}
				}
				else {
					CATAPULT_LOG( error ) << "Could Not Find Drive to Verify " << driveKey << " " << blockHash;
				}
			}
        }

        void updateDriveReplicators(const Key& driveKey) {
			CATAPULT_LOG(debug) << "update drive replicators" << driveKey;

        	auto replicators = castReplicatorKeys<sirius::Key>(m_storageState.getDriveReplicators(driveKey));
//			m_pReplicator->asyncSetDriveReplicators(driveKey.array(), replicators);
        }

        void updateShardDonator(const Key& driveKey) {
        	CATAPULT_LOG(debug) << "update shardDonator" << driveKey;

        	auto donatorShard = castReplicatorKeys<sirius::Key>(m_storageState.getDonatorShard(driveKey, m_keyPair.publicKey()));
			m_pReplicator->asyncSetShardDonator(driveKey.array(), donatorShard);
		}

        void updateShardRecipient(const Key& driveKey) {
        	CATAPULT_LOG(debug) << "update shardRecipient" << driveKey;

        	auto recipientShard = castReplicatorKeys<sirius::Key>(m_storageState.getRecipientShard(driveKey, m_keyPair.publicKey()));
			m_pReplicator->asyncSetShardRecipient(driveKey.array(), recipientShard);
		}

		void updateDownloadChannelReplicators(const Hash256& channelId) {
        	CATAPULT_LOG(debug) << "update channel replicators" << channelId;

        	auto replicators = castReplicatorKeys<sirius::Key>(m_storageState.getDriveReplicators(channelId.array()));
//        	m_pReplicator->asyncSetDownloadChannelShard(channelId.array(), replicators);
        }

		void updateDriveDownloadChannels(const Key& driveKey) {
        	CATAPULT_LOG(debug) << "update drive channels replicators" << driveKey;

			auto driveChannels = m_storageState.getDriveChannels(driveKey);

			for (const auto& channel: driveChannels) {
				updateDownloadChannelReplicators(channel);
			}
		}

		void anotherReplicatorOnboarded(const Key& replicatorKey)
		{
			auto drives = m_storageState.getReplicatorDriveKeys(replicatorKey);
			for (const auto& driveKey: drives) {
				if (m_alreadyAddedDrives.find(driveKey) != m_alreadyAddedDrives.end()) {
					updateDriveReplicators(driveKey);
					updateShardDonator(driveKey);
					updateShardRecipient(driveKey);
				}
			}

			auto onboardedReplicatorChannels = m_storageState.getReplicatorChannelIds(replicatorKey);
			auto myChannels = m_storageState.getReplicatorChannelIds(m_keyPair.publicKey());
			std::set<Hash256> myChannelsSet = {myChannels.begin(), myChannels.end()};
			for (const auto& channelId: onboardedReplicatorChannels) {
				if (myChannelsSet.find(channelId) != myChannelsSet.end())
				{
					updateDownloadChannelReplicators(channelId);
				}
				else if (m_alreadyAddedChannels.find(channelId) != m_alreadyAddedChannels.end())
				{
					// The Replicator has been removed from the Channel with the transaction
//					m_pReplicator->asyncRemoveDownloadChannelInfo(channelId.array());
					m_alreadyAddedChannels.erase(channelId);
				}
			}
		}

		void storageBlockPublished(const Hash256& blockHash) {
			auto drives = m_storageState.getReplicatorDriveKeys(m_keyPair.publicKey());

			auto height = m_storageState.getChainHeight();

			std::vector<Key> newlyAddedDrives;
			std::vector<Key> newlyRemovedDrives;
			for (const auto& blockchainDriveKey: drives) {
				if (m_alreadyAddedDrives.find(blockchainDriveKey) == m_alreadyAddedDrives.end()) {
					// We are assigned to Drive, but it is not added
					addDrive(blockchainDriveKey);
					m_alreadyAddedDrives[blockchainDriveKey] = height;
				}
			}
			for (const auto& [addedDriveKey, _]: m_alreadyAddedDrives) {
				if (m_storageState.driveExists(addedDriveKey)) {
					m_pReplicator->asyncCloseDrive(
							addedDriveKey.array(),
							blockHash.array());
					m_alreadyAddedDrives.erase(addedDriveKey);
				}
			}
		}

		void downloadBlockPublished(const Hash256& blockHash) {
			for (const auto& [channelId, _]: m_alreadyAddedChannels) {
				if (!m_storageState.downloadChannelExists(channelId)) {
					auto driveKey = Key(); // m_storageState.getC
					m_pReplicator->asyncRemoveDownloadChannelInfo(driveKey.array(), channelId.array());
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

			auto channelClosed = !m_storageState.downloadChannelExists(downloadChannelId);
			m_pReplicator->asyncDownloadApprovalTransactionHasBeenPublished(approvalTrigger.array(), downloadChannelId.array(), channelClosed);
        }

        void notifyTransactionStatus(const Hash256& hash, uint32_t status) {
            m_transactionStatusHandler.handle(hash, status);
        }

        void stop() {
			m_pReplicator.reset();
        }

    private:
		template<class T>
		std::vector<T> castReplicatorKeys(const std::vector<Key>& keys) {
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
        std::unique_ptr<ReplicatorEventHandler> m_pReplicatorEventHandler;
        TransactionStatusHandler m_transactionStatusHandler;
		const std::vector<ionet::Node>& m_bootstrapReplicators;

		// The fields are needed to generate correct events
		std::map<Key, Height> m_alreadyAddedDrives;
		std::map<Hash256, Height> m_alreadyAddedChannels;
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
            uint64_t dataSizeMegabytes) {
        if (m_pImpl)
			m_pImpl->addDriveModification(driveKey, downloadDataCdi, modificationId, owner, dataSizeMegabytes);
    }

    void ReplicatorService::removeDriveModification(const Key& driveKey, const Hash256& dataModificationId) {
        if (m_pImpl)
            m_pImpl->removeDriveModification(driveKey, dataModificationId);
    }

    void ReplicatorService::addDownloadChannel(const Hash256& channelId) {
        if (m_pImpl)
            m_pImpl->addDownloadChannel(channelId);
    }

    void ReplicatorService::increaseDownloadChannelSize(const Hash256& channelId) {
        if (m_pImpl)
            m_pImpl->increaseDownloadChannelSize(channelId);
    }

    bool ReplicatorService::isAssignedToChannel(const Hash256& channelId) {
    	if (m_pImpl)
    		m_pImpl->isAssignedToChannel(channelId);
    }

    void ReplicatorService::initiateDownloadApproval(const Hash256& channelId, const Hash256& eventHash) {
        if (m_pImpl)
			m_pImpl->initiateDownloadApproval(channelId, eventHash);
    }

    void ReplicatorService::addDrive(const Key& driveKey) {
        if (m_pImpl)
            m_pImpl->addDrive(driveKey);
    }

	void ReplicatorService::removeDrive(const Key& driveKey) {
    	if (m_pImpl)
    		m_pImpl->removeDrive(driveKey);
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

    void ReplicatorService::storageBlockPublished(const Hash256& blockHash) {
		if (m_pImpl) {
			m_pImpl->storageBlockPublished(blockHash);
		}
	}

	void ReplicatorService::downloadBlockPublished(const Hash256& blockHash) {
    	if (m_pImpl) {
    		m_pImpl->downloadBlockPublished(blockHash);
		}
	}

    std::optional<Height> ReplicatorService::driveAddedAt(const Key& driveKey) {
    	if (m_pImpl)
    		return m_pImpl->driveAddedAt(driveKey);
		return {};
	}

	std::optional<Height> ReplicatorService::channelAddedAt(const Hash256& channelId) {
    	if (m_pImpl)
    		return m_pImpl->channelAddedAt(channelId);
    	return {};
    }

	void ReplicatorService::exploreNewDrives() {
    	if (m_pImpl)
    		return m_pImpl->exploreNewDrives();
	}

	void ReplicatorService::processVerifications(const Hash256& blockHash) {
        if (m_pImpl)
        	m_pImpl->processVerifications(blockHash);
    }

    void ReplicatorService::updateDriveReplicators(const Key& driveKey) {
		if (m_pImpl) {
			m_pImpl->updateDriveReplicators(driveKey);
		}
	}

	void ReplicatorService::updateShardDonator(const Key& driveKey) {
    	if (m_pImpl) {
    		m_pImpl->updateShardDonator(driveKey);
    	}
	}

	void ReplicatorService::updateShardRecipient(const Key& driveKey) {
    	if (m_pImpl) {
    		m_pImpl->updateShardRecipient(driveKey);
    	}
	}

	void ReplicatorService::updateDriveDownloadChannels(const Key& driveKey) {
    	if (m_pImpl) {
    		m_pImpl->updateDriveDownloadChannels(driveKey);
    	}
	}

    void ReplicatorService::notifyTransactionStatus(const Hash256& hash, uint32_t status) {
        if (m_pImpl)
            m_pImpl->notifyTransactionStatus(hash, status);
    }

    void ReplicatorService::anotherReplicatorOnboarded(const Key& replicatorKey) {
    	if (m_pImpl)
    		m_pImpl->anotherReplicatorOnboarded(replicatorKey);
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
