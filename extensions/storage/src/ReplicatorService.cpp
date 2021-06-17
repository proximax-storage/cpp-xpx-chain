/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ReplicatorService.h"
#include "StoragePacketHandlers.h"
#include "sdk/src/builders/DataModificationApprovalBuilder.h"
#include "catapult/extensions/ServiceLocator.h"
#include "catapult/extensions/ServiceState.h"
#include "sdk/src/extensions/TransactionExtensions.h"
#include "catapult/utils/NetworkTime.h"
#include <libtorrent/alert_types.hpp>
#include <filesystem>

namespace catapult { namespace storage {

	// region - replicator service registrar

	namespace {
		constexpr auto Service_Name = "replicator";

		class ReplicatorServiceRegistrar : public extensions::ServiceRegistrar {
		public:
			explicit ReplicatorServiceRegistrar(std::shared_ptr<storage::ReplicatorService> pReplicatorService)
				: m_pReplicatorService(std::move(pReplicatorService))
			{}

			extensions::ServiceRegistrarInfo info() const override {
				return { "Replicator", extensions::ServiceRegistrarPhase::Post_Range_Consumers };
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
					for (const auto& consumerPair : replicatorData.Consumers) {
						for (const auto& drivePair : consumerPair.second)
							m_pReplicatorService->addConsumer(consumerPair.first, drivePair.first, drivePair.second);
					}

					for (const auto& pair : replicatorData.Drives) {
						m_pReplicatorService->addDrive(pair.first, pair.second);
						const auto& driveModifications = replicatorData.DriveModifications[pair.first];
						for (const auto& driveModification : driveModifications)
							m_pReplicatorService->addDriveModification(pair.first, driveModification.first, driveModification.second);
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

		auto AsString(const Key &driveKey) {
			std::ostringstream out;
			out << driveKey;
			return out.str();
		}
	}

	class ReplicatorService::Impl : public std::enable_shared_from_this<ReplicatorService::Impl> {
	public:
		Impl(
			crypto::KeyPair&& keyPair,
			model::NetworkIdentifier networkIdentifier,
			const GenerationHash& generationHash,
			handlers::TransactionRangeHandler transactionRangeHandler,
			StorageConfiguration&& storageConfig)
				: m_stopped(false)
				, m_driveModificationInProgress(false)
				, m_keyPair(std::move(keyPair))
				, m_networkIdentifier(networkIdentifier)
				, m_generationHash(generationHash)
				, m_transactionRangeHandler(std::move(transactionRangeHandler))
				, m_storageConfig(std::move(storageConfig))
		{}

	public:
		Key replicatorKey() const {
			return m_keyPair.publicKey();
		}

		std::map<Key, std::map<Key, uint64_t>>& consumers() {
			return m_consumers;
		}

		void addDrive(const Key& driveKey, size_t driveSize) {
			CATAPULT_LOG(debug) << "adding drive " << driveKey;

			std::unique_lock<std::mutex> lock(m_mutex);

			if (m_stopped)
				return;

			if (m_drives.find(driveKey) != m_drives.end())
				CATAPULT_THROW_INVALID_ARGUMENT_1("drive already added", driveKey);

			m_drives[driveKey] = sirius::drive::createDefaultDrive(
				session(),
				m_storageConfig.StorageDirectory,
				m_storageConfig.SandboxDirectory,
				AsString(driveKey),
				driveSize);
		}

		void removeDrive(const Key& driveKey) {
			CATAPULT_LOG(debug) << "removing drive " << driveKey;

			std::unique_lock<std::mutex> lock(m_mutex);

			if (m_stopped)
				return;

			if (m_drives.find(driveKey) == m_drives.end())
				CATAPULT_THROW_INVALID_ARGUMENT_1("drive not found", driveKey);

			// TODO: graceful drive shutdown.
			// m_drives.at(driveKey)->shutdown();
			m_drives.erase(driveKey);
			m_driveModifications.erase(driveKey);
		}

		void addDriveModification(const Key& driveKey, const Hash256& dataModificationId, const Hash256& rootHash) {
			CATAPULT_LOG(debug) << "adding drive modification:\ndrive: " << driveKey << "\nmodification id: " << dataModificationId
				<< "\n root hash: " << rootHash;

			{
				std::unique_lock<std::mutex> lock(m_mutex);

				if (m_stopped)
					return;

				if (m_drives.find(driveKey) == m_drives.end())
					CATAPULT_THROW_INVALID_ARGUMENT_1("drive not found", driveKey);

				m_driveModifications[driveKey].emplace_back(dataModificationId, rootHash.array());
			}

			bool modificationStarted = false;
			if (m_driveModificationInProgress.compare_exchange_strong(modificationStarted, true))
				startDriveModification(driveKey);
		}

		void removeDriveModification(const Key& driveKey, const Hash256& dataModificationId) {
			CATAPULT_LOG(debug) << "removing drive modification:\ndrive: " << driveKey << "\nmodification id: " << dataModificationId;

			std::unique_lock<std::mutex> lock(m_mutex);

			if (m_stopped)
				return;

			if (m_driveModifications.find(driveKey) == m_driveModifications.end())
				CATAPULT_THROW_INVALID_ARGUMENT_1("drive not found", driveKey);

			auto& driveModifications = m_driveModifications.at(driveKey);
			auto iter = std::find_if(driveModifications.begin(), driveModifications.end(), [dataModificationId] (const auto& pair) {
				return (pair.first == dataModificationId);
			});

			if (iter == driveModifications.end())
				CATAPULT_THROW_INVALID_ARGUMENT_2("drive modification not found", driveKey, dataModificationId);

			driveModifications.erase(iter);
		}

		void addConsumer(const Key& consumer, const Key& driveKey, uint64_t downloadSize) {
			CATAPULT_LOG(debug) << "adding consumer:\ndrive: " << driveKey << "\nconsumer: " << consumer
				<< "\n download size: " << downloadSize;

			m_consumers[consumer][driveKey] += downloadSize;
		}

		void removeConsumer(const Key& consumer, const Key& driveKey) {
			CATAPULT_LOG(debug) << "removing consumer:\ndrive: " << driveKey << "\nconsumer: " << consumer;

			if (m_consumers.find(consumer) == m_consumers.end())
				CATAPULT_THROW_INVALID_ARGUMENT_1("consumer not found", consumer);

			auto consumerData = m_consumers.at(consumer);
			if (consumerData.find(driveKey) == consumerData.end())
				CATAPULT_THROW_INVALID_ARGUMENT_2("consumer is not added to drive", consumer, driveKey);

			consumerData.erase(driveKey);
			if (consumerData.empty())
				m_consumers.erase(consumer);
		}

		FileNames startDownloadFiles(const Key& consumer, const Key& driveKey, FileNames&& fileNames) {
			CATAPULT_LOG(debug) << "start downloading files:\ndrive: " << driveKey << "\nconsumer: " << consumer;

			FileNames remainingFiles = std::move(fileNames);
			for (const auto& fileName : remainingFiles)
				CATAPULT_LOG(debug) << "file: " << fileName;

			if (m_consumers.find(consumer) == m_consumers.end()) {
				CATAPULT_LOG(warning) << "consumer " << consumer << " not found ";
				return remainingFiles;
			}

			auto consumerData = m_consumers.at(consumer);
			if (consumerData.find(driveKey) == consumerData.end()) {
				CATAPULT_LOG(warning) << "consumer " << consumer << " is not added to drive " << driveKey;
				return remainingFiles;
			}

			auto& downloadLimit = consumerData.at(driveKey);
			std::filesystem::path driveDirectory = m_storageConfig.StorageDirectory;
			auto fileCount = remainingFiles.size();
			for (auto i = 0u; i < fileCount; ++i) {
				auto fileName = *remainingFiles.begin();
				std::filesystem::path file = driveDirectory / fileName;
				auto fileSize = std::filesystem::file_size(file);
				if (fileSize > downloadLimit) {
					CATAPULT_LOG(warning) << "consumer's " << consumer << " download limit " << downloadLimit << " exceeded with " << fileSize << " bytes";
					return remainingFiles;
				}

				std::filesystem::path torrentFile = driveDirectory / (fileName + ".torrent");
				sirius::drive::createTorrentFile(file, file.parent_path(), torrentFile);
				auto handle = session()->addTorrentFileToSession(torrentFile, file.parent_path());
				m_fileDownloads.emplace(file, handle);
				remainingFiles.erase(remainingFiles.begin());

				downloadLimit -= fileSize;
				if (!downloadLimit) {
					consumerData.erase(driveKey);
					if (consumerData.empty())
						m_consumers.erase(consumer);

					return remainingFiles;
				}
			}

			return remainingFiles;
		}

		FileNames stopDownloadFiles(const Key& driveKey, FileNames&& fileNames) {
			CATAPULT_LOG(debug) << "stop downloading files:\ndrive: " << driveKey;

			FileNames allFiles = std::move(fileNames);
			FileNames notFoundFiles;
			for (const auto& fileName : allFiles)
				CATAPULT_LOG(debug) << "file: " << fileName;

			std::filesystem::path driveDirectory = m_storageConfig.StorageDirectory;
			for (const auto& fileName : allFiles) {
				std::filesystem::path file = driveDirectory / fileName;
				if (m_fileDownloads.find(file) == m_fileDownloads.end()) {
					CATAPULT_LOG(warning) << "file download " << fileName << " not found in drive " << driveKey;
					notFoundFiles.push_back(fileName);
					continue;
				}

				session()->removeTorrentFromSession(m_fileDownloads.at(file));
				m_fileDownloads.erase(file);
			}

			return notFoundFiles;
		}

		void closeDrive(const Key& driveKey) {
			CATAPULT_LOG(debug) << "closing drive " << driveKey;

			std::unique_lock<std::mutex> lock(m_mutex);

			if (m_stopped)
				return;

			if (m_drives.find(driveKey) == m_drives.end())
				CATAPULT_THROW_INVALID_ARGUMENT_1("drive not found", driveKey);
		}

		void shutdown() {
			std::unique_lock<std::mutex> lock(m_mutex);

			if (m_stopped)
				return;

			m_stopped = true;

			if (m_pSession)
				m_pSession->endSession();
		}

	private:
		void sendDataModificationApprovalTransaction(const Key& driveKey, const Hash256& dataModificationId, const Hash256& driveRootHash) {
			CATAPULT_LOG(debug) << "sending data modification approval transaction:\ndrive: " << driveKey << "\nmodification id: " << dataModificationId
				<< "\n root hash: " << driveRootHash;

			builders::DataModificationApprovalBuilder builder(m_networkIdentifier, m_keyPair.publicKey());
			builder.setDriveKey(driveKey);
			builder.setDataModificationId(dataModificationId);
			builder.setFileStructureCdi(driveRootHash);
			auto pTransaction = utils::UniqueToShared(builder.build());
			pTransaction->Deadline = utils::NetworkTime() + Timestamp(m_storageConfig.TransactionTimeout.millis());
			extensions::TransactionExtensions(m_generationHash).sign(m_keyPair, *pTransaction);
			auto range = model::TransactionRange::FromEntity(pTransaction);
			m_transactionRangeHandler({ std::move(range), m_keyPair.publicKey() });
		}

		void startDriveModification(const Key& driveKey) {
			Hash256 dataModificationId;
			sirius::drive::InfoHash infoHash;
			std::shared_ptr<sirius::drive::Drive> pDrive;

			{
				std::unique_lock<std::mutex> lock(m_mutex);

				auto& driveModifications = m_driveModifications.at(driveKey);
				if (m_stopped || driveModifications.empty()) {
					m_driveModificationInProgress = false;
					return;
				}

				dataModificationId = driveModifications.front().first;
				infoHash = driveModifications.front().second;
				driveModifications.erase(driveModifications.begin());
				pDrive = m_drives.at(driveKey);
			}

			pDrive->startModifyDrive(infoHash, [pThis = shared_from_this(), dataModificationId, driveKey, pDrive](
				sirius::drive::modify_status::code code,
				sirius::drive::InfoHash rootHash,
				std::string error) {
				Hash256 driveRootHash(pDrive->rootDriveHash());
				switch (code) {
					case sirius::drive::modify_status::update_completed: {
						CATAPULT_LOG(debug) << " drive update completed:\n drive key: " << driveKey << "\n root hash: " << driveRootHash;
						pThis->sendDataModificationApprovalTransaction(driveKey, dataModificationId, driveRootHash);
						break;
					}
					case sirius::drive::modify_status::sandbox_root_hash: {
						Hash256 driveRootHash(rootHash);
						CATAPULT_LOG(debug) << " drive modified in sandbox:\n drive key: " << driveKey << "\n root hash: " << driveRootHash;
						break;
					}
					case sirius::drive::modify_status::failed: {
						CATAPULT_LOG(error) << " drive modification failed:\n drive key: " << driveKey << "\n error: " << error;
						// TODO: send data modification fail transaction?
						break;
					}
						// TODO: graceful drive shutdown.
//					case sirius::drive::modify_status::aborted: {
//						CATAPULT_LOG(error) << " drive modification aborted:\n drive key: " << driveKey;
//						m_driveModificationInProgress = false;
//						return;
//					}
				}

				pThis->startDriveModification(driveKey);
			});
		}

		std::shared_ptr<sirius::drive::Session> session() {
			if (!m_pSession) {
				m_pSession = sirius::drive::createDefaultSession(Replicator_Host + m_storageConfig.Port, [](libtorrent::alert* pAlert) {
					if ( pAlert->type() == lt::listen_failed_alert::alert_type ) {
						CATAPULT_LOG(error) << "replicator session alert: " << pAlert->message();
					}
				});
			}

			return m_pSession;
		}

	private:
		std::shared_ptr<sirius::drive::Session> m_pSession;
		std::map<Key, std::shared_ptr<sirius::drive::Drive>> m_drives;
		std::map<Key, std::vector<std::pair<Hash256, sirius::drive::InfoHash>>> m_driveModifications;
		std::map<Key, std::map<Key, uint64_t>> m_consumers;
		std::map<std::string, libtorrent::torrent_handle> m_fileDownloads;
		std::mutex m_mutex;
		bool m_stopped;
		std::atomic_bool m_driveModificationInProgress;
		crypto::KeyPair m_keyPair;
		model::NetworkIdentifier m_networkIdentifier;
		GenerationHash m_generationHash;
		handlers::TransactionRangeHandler m_transactionRangeHandler;
		StorageConfiguration m_storageConfig;
	};

	void ReplicatorService::start() {
		if (!m_pImpl) {
			m_pImpl = std::make_shared<ReplicatorService::Impl>(
				std::move(m_keyPair),
				m_networkIdentifier,
				m_generationHash,
				m_transactionRangeHandler,
				std::move(m_storageConfig));
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

	void ReplicatorService::addDrive(const Key& driveKey, size_t driveSize) {
		if (m_pImpl)
			m_pImpl->addDrive(driveKey, driveSize);
	}

	void ReplicatorService::removeDrive(const Key& driveKey) {
		if (m_pImpl)
			m_pImpl->removeDrive(driveKey);
	}

	void ReplicatorService::addDriveModification(const Key& driveKey, const Hash256& dataModificationId, const Hash256& rootHash) {
		if (m_pImpl)
			m_pImpl->addDriveModification(driveKey, dataModificationId, rootHash);
	}

	void ReplicatorService::removeDriveModification(const Key& driveKey, const Hash256& dataModificationId) {
		if (m_pImpl)
			m_pImpl->removeDriveModification(driveKey, dataModificationId);
	}

	void ReplicatorService::addConsumer(const Key& consumer, const Key& driveKey, uint64_t downloadSize) {
		if (m_pImpl)
			m_pImpl->addConsumer(consumer, driveKey, downloadSize);
	}

	void ReplicatorService::removeConsumer(const Key& consumer, const Key& driveKey) {
		if (m_pImpl)
			m_pImpl->removeConsumer(consumer, driveKey);
	}

	FileNames ReplicatorService::startDownloadFiles(const Key& consumer, const Key& driveKey, FileNames&& fileNames) {
		if (m_pImpl)
			return m_pImpl->startDownloadFiles(consumer, driveKey, std::move(fileNames));

		return std::move(fileNames);
	}

	FileNames ReplicatorService::stopDownloadFiles(const Key& driveKey, FileNames&& fileNames) {
		if (m_pImpl)
			return m_pImpl->stopDownloadFiles(driveKey, std::move(fileNames));

		return std::move(fileNames);
	}

	void ReplicatorService::closeDrive(const Key& driveKey) {
		if (m_pImpl)
			m_pImpl->closeDrive(driveKey);
	}

	// endregion
}}
