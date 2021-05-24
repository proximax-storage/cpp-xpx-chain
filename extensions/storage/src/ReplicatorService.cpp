/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ReplicatorService.h"
#include "StoragePacketHandlers.h"
#include "catapult/builders/DataModificationApprovalBuilder.h"
#include "catapult/extensions/ServiceLocator.h"
#include "catapult/extensions/ServiceState.h"
#include "catapult/extensions/TransactionExtensions.h"
#include "catapult/utils/NetworkTime.h"
#include <libtorrent/alert_types.hpp>
#include <filesystem>

namespace catapult { namespace storage {

	// region - replicator service registrar

	namespace {
		constexpr auto Service_Name = "replicator";
		constexpr auto Replicator_Host = "120.0.0.1";

		auto getDriveDirectoryName(const Key& driveKey) {
			std::ostringstream out;
			out << driveKey;
			return out.str();
		}

		auto getDriveDirectory(const Key& driveKey, const std::string& parentDirectory) {
			std::filesystem::path driveDirectory = parentDirectory;
			driveDirectory /= getDriveDirectoryName(driveKey);
			return driveDirectory;
		}

		auto getDriveDirectory(const std::string& driveDirectoryName, const std::string& parentDirectory) {
			std::filesystem::path driveDirectory = parentDirectory;
			driveDirectory /= driveDirectoryName;
			return driveDirectory;
		}

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
				auto& storageState = state.pluginManager().getStorageState();
				if (storageState.isReplicatorRegistered(m_pReplicatorService->replicatorKey())) {
					locator.registerService(Service_Name, m_pReplicatorService);

					auto replicatorData = storageState.getReplicatorData(m_pReplicatorService->replicatorKey(), state.cache());
					m_pReplicatorService->consumers() = replicatorData.Consumers;
					for (const auto& pair : replicatorData.Drives) {
						m_pReplicatorService->addDrive(pair.first, pair.second);
						const auto& driveModifications = replicatorData.DriveModifications.at(pair.first);
						for (const auto& driveModification : driveModifications)
							m_pReplicatorService->addDriveModification(pair.first, driveModification.first, driveModification.second);
					}

					m_pReplicatorService->setTransactionRangeHandler(state.hooks().transactionRangeConsumerFactory()(disruptor::InputSource::Local));
					RegisterStartDownloadFilesHandler(m_pReplicatorService, state.packetHandlers());
					RegisterStopDownloadFilesHandler(m_pReplicatorService, state.packetHandlers());
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
	}

	void ReplicatorService::shutdown() {
		std::unique_lock<std::mutex> lock(m_mutex);

		if (m_stopped)
			return;

		m_stopped = true;

		if (m_pSession)
			m_pSession->endSession();
	}

	void ReplicatorService::addDrive(const Key& driveKey, size_t driveSize) {
		std::unique_lock<std::mutex> lock(m_mutex);

		if (m_stopped)
			return;

		if (m_drives.find(driveKey) != m_drives.end())
			CATAPULT_THROW_INVALID_ARGUMENT_1("drive already added", driveKey);

		auto driveDirectoryName = getDriveDirectoryName(driveKey);
		m_drives[driveKey] = sirius::drive::createDefaultDrive(
			session(),
			getDriveDirectory(driveDirectoryName, m_storageConfig.StorageDirectory),
			getDriveDirectory(driveDirectoryName, m_storageConfig.SandboxDirectory),
			driveDirectoryName,
			driveSize);
	}

	void ReplicatorService::removeDrive(const Key& driveKey) {
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

	void ReplicatorService::addDriveModification(const Key& driveKey, const Hash256& dataModificationId, const Hash256& rootHash) {
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

	void ReplicatorService::removeDriveModification(const Key& driveKey, const Hash256& dataModificationId) {
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

	void ReplicatorService::sendDataModificationApprovalTransaction(
			const Key& driveKey,
			const Hash256& dataModificationId,
			const Hash256& driveRootHash) {
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

	void ReplicatorService::startDriveModification(const Key& driveKey) {
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
//					pThis->sendDataModificationApprovalTransaction(driveKey, dataModificationId, driveRootHash);
					break;
				}
				// TODO: graceful drive shutdown.
//				case sirius::drive::modify_status::aborted: {
//					CATAPULT_LOG(error) << " drive modification aborted:\n drive key: " << driveKey;
//					m_driveModificationInProgress = false;
//					return;
//				}
			}

			pThis->startDriveModification(driveKey);
		});
	}

	void ReplicatorService::addConsumer(const Key& consumer, const Key& driveKey, uint64_t downloadSize) {
		m_consumers[consumer][driveKey] += downloadSize;
	}

	void ReplicatorService::removeConsumer(const Key& consumer, const Key& driveKey) {
		if (m_consumers.find(consumer) == m_consumers.end())
			CATAPULT_THROW_INVALID_ARGUMENT_1("consumer not found", consumer);

		auto consumerData = m_consumers.at(consumer);
		if (consumerData.find(driveKey) == consumerData.end())
			CATAPULT_THROW_INVALID_ARGUMENT_2("consumer is not added to drive", consumer, driveKey);

		consumerData.erase(driveKey);
		if (consumerData.empty())
			m_consumers.erase(consumer);
	}

	FileNames ReplicatorService::startDownloadFiles(const Key& consumer, const Key& driveKey, FileNames&& fileNames) {
		FileNames remainingFiles = std::move(fileNames);
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
		auto driveDirectory = getDriveDirectory(driveKey, m_storageConfig.StorageDirectory);
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

	FileNames ReplicatorService::stopDownloadFiles(const Key& driveKey, FileNames&& fileNames) {
		FileNames allFiles = std::move(fileNames);
		FileNames notFoundFiles;
		auto driveDirectory = getDriveDirectory(driveKey, m_storageConfig.StorageDirectory);
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

	std::shared_ptr<sirius::drive::Session> ReplicatorService::session() {
		if (!m_pSession) {
			m_pSession = sirius::drive::createDefaultSession(Replicator_Host + m_storageConfig.Port, [](libtorrent::alert* pAlert) {
				if ( pAlert->type() == lt::listen_failed_alert::alert_type ) {
					CATAPULT_LOG(error) << "replicator session alert: " << pAlert->message();
				}
			});
		}

		return m_pSession;
	}

	// endregion
}}
