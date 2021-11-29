/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once

#include "catapult/state/StorageState.h"
#include "StorageConfiguration.h"
#include "catapult/crypto/KeyPair.h"
#include "catapult/extensions/ServiceRegistrar.h"
#include "catapult/types.h"
#include "drive/FlatDrive.h"
#include "drive/Session.h"
#include "ReplicatorEventHandler.h"

namespace catapult { namespace storage {

	using FileNames = std::vector<std::string>;

	class ReplicatorService {
	public:
		ReplicatorService(
				crypto::KeyPair&& keyPair,
				model::NetworkIdentifier networkIdentifier,
				const GenerationHash& generationHash,
				StorageConfiguration&& storageConfig,
				state::StorageState& storageState)
			: m_keyPair(std::move(keyPair))
			, m_networkIdentifier(networkIdentifier)
			, m_generationHash(generationHash)
			, m_storageConfig(std::move(storageConfig))
			, m_storageState(storageState)
		{}

	public:
		void setTransactionRangeHandler(handlers::TransactionRangeHandler transactionRangeHandler) {
			m_transactionRangeHandler = std::move(transactionRangeHandler);
		}

		Key replicatorKey() const;

		void start();

		void addDriveModification(const Key& driveKey,
								  const Hash256& downloadDataCdi,
								  const Hash256& modificationId,
								  const Key& owner,
								  uint64_t dataSize);

		void removeDriveModification(const Key& driveKey, const Hash256& dataModificationId);

        // TODO looks incorrect
		void addDriveChannel(const Hash256& channelId,
							 const Key& driveKey,
							 size_t prepaidDownloadSize,
							 const std::vector<Key>& consumers);

		// TODO update download channel size
		void increaseDownloadChannelSize(const Hash256& channelId, size_t increasedDownloadSize);

		void closeDriveChannel(const Hash256& channelId);

		void addDrive(const Key& driveKey, uint64_t driveSize, uint64_t usedSize, const utils::KeySet& replicators);

		bool driveExist(const Key& driveKey);

		void closeDrive(const Key& driveKey, const Hash256& transactionHash);

		void shutdown();

		void terminate();

	private:
		class Impl;
		std::shared_ptr<Impl> m_pImpl;

		crypto::KeyPair m_keyPair;
		model::NetworkIdentifier m_networkIdentifier;
		GenerationHash m_generationHash;
		StorageConfiguration m_storageConfig;
		handlers::TransactionRangeHandler m_transactionRangeHandler;
		state::StorageState& m_storageState;
	};

	/// Creates a registrar for the replicator service.
	DECLARE_SERVICE_REGISTRAR(Replicator)(std::shared_ptr<storage::ReplicatorService> pReplicatorService);
}}
