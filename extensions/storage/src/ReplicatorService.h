/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "StorageConfiguration.h"
#include "catapult/crypto/KeyPair.h"
#include "catapult/extensions/ServerHooks.h"
#include "catapult/extensions/ServiceRegistrar.h"
#include "catapult/types.h"
#include "drive/Drive.h"
#include "drive/Session.h"
#include <mutex>

namespace catapult { namespace storage {

	using FileNames = std::vector<std::string>;

	class ReplicatorService {
	public:
		ReplicatorService(
				crypto::KeyPair&& keyPair,
				model::NetworkIdentifier networkIdentifier,
				const GenerationHash& generationHash,
				StorageConfiguration&& storageConfig)
			: m_keyPair(std::move(keyPair))
			, m_networkIdentifier(networkIdentifier)
			, m_generationHash(generationHash)
			, m_storageConfig(std::move(storageConfig))
		{}

	public:
		void setTransactionRangeHandler(handlers::TransactionRangeHandler transactionRangeHandler) {
			m_transactionRangeHandler = std::move(transactionRangeHandler);
		}

		Key replicatorKey() const;

		void start();

		void addDrive(const Key& driveKey, size_t driveSize);
		void removeDrive(const Key& driveKey);

		void addDriveModification(const Key& driveKey, const Hash256& dataModificationId, const Hash256& rootHash);
		void removeDriveModification(const Key& driveKey, const Hash256& dataModificationId);

		void addConsumer(const Key& consumer, const std::vector<Key> listOfPublicKeys, const uint64_t downloadSize);
		void removeConsumer(const Key& consumer);

		FileNames startDownloadFiles(const Key& consumer, const Key& driveKey, FileNames&& fileNames);
		FileNames stopDownloadFiles(const Key& driveKey, FileNames&& fileNames);

		void shutdown();

	private:
		class Impl;
		std::shared_ptr<Impl> m_pImpl;

		crypto::KeyPair m_keyPair;
		model::NetworkIdentifier m_networkIdentifier;
		GenerationHash m_generationHash;
		StorageConfiguration m_storageConfig;
		handlers::TransactionRangeHandler m_transactionRangeHandler;
	};

	/// Creates a registrar for the replicator service.
	DECLARE_SERVICE_REGISTRAR(Replicator)(std::shared_ptr<storage::ReplicatorService> pReplicatorService);
}}
