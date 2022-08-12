/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/model/Elements.h"
#include "catapult/types.h"
#include "catapult/utils/ArraySet.h"
#include "catapult/utils/NonCopyable.h"
#include <vector>

namespace catapult { namespace cache { class CatapultCache; } }

namespace catapult { namespace state {

	struct DataModification {
		Hash256 Id;
		Key Owner;
		Key DriveKey;
		Hash256 DownloadDataCdi;
		uint64_t ExpectedUploadSize;
		uint64_t ActualUploadSize;
	};

	struct ApprovedDataModification : DataModification {
		std::vector<Key> Signers;
		uint64_t UsedSize;
	};

	struct Drive {
		Key Id;
		Key Owner;
		Hash256 RootHash;
		uint64_t Size;
		utils::SortedKeySet Replicators;
		std::vector<DataModification> DataModifications;
	};

	struct DownloadChannel {
		Hash256 Id;
		uint64_t DownloadSizeMegabytes;
		std::vector<Key> Consumers;
		std::vector<Key> Replicators;
		Key DriveKey;
		std::optional<Hash256> ApprovalTrigger;
	};

	struct DriveVerification {
		Key DriveKey;
		uint32_t Duration;
		bool Expired;
		Hash256 VerificationTrigger;
		Hash256 RootHash;
		std::vector<std::set<Key>> Shards;
	};

	struct ModificationShard {
		std::map<Key, uint64_t> m_actualShardMembers;
		std::map<Key, uint64_t> m_formerShardMembers;
		uint64_t m_ownerUpload = 0;
	};

	/// Interface for storage state.
	class StorageState : public utils::NonCopyable {
	public:
		virtual ~StorageState() = default;

	public:
		void setCache(cache::CatapultCache* pCache) {
			m_pCache = pCache;
		}

		void setLastBlockElementSupplier(const model::BlockElementSupplier& lastBlockElementSupplier) {
			if (!!m_lastBlockElementSupplier)
				CATAPULT_THROW_RUNTIME_ERROR("last block element supplier already set");

			m_lastBlockElementSupplier = lastBlockElementSupplier;
		}

		const model::BlockElementSupplier& lastBlockElementSupplier() const {
			if (!m_lastBlockElementSupplier)
				CATAPULT_THROW_RUNTIME_ERROR("last block element supplier not set");

			return m_lastBlockElementSupplier;
		}

	public:
		virtual Height getChainHeight() = 0;

		virtual bool isReplicatorRegistered(const Key& key) = 0;

		virtual bool driveExists(const Key& driveKey) = 0;
		virtual Drive getDrive(const Key& driveKey) = 0;
		virtual bool isReplicatorAssignedToDrive(const Key& key, const Key& driveKey) = 0;
		virtual bool isReplicatorAssignedToChannel(const Key& key, const Hash256& channelId) = 0;
		virtual std::vector<Key> getReplicatorDriveKeys(const Key& replicatorKey) = 0;
		virtual std::set<Hash256> getReplicatorChannelIds(const Key& replicatorKey) = 0;
		virtual std::vector<Drive> getReplicatorDrives(const Key& replicatorKey) = 0;
		virtual std::vector<Key> getDriveReplicators(const Key& driveKey) = 0;
		virtual std::vector<Hash256> getDriveChannels(const Key& driveKey) = 0;
		virtual std::vector<Key> getDonatorShard(const Key& driveKey, const Key& replicatorKey) = 0;
		virtual ModificationShard getDonatorShardExtended(const Key& driveKey, const Key& replicatorKey) = 0;
		virtual std::vector<Key> getRecipientShard(const Key& driveKey, const Key& replicatorKey) = 0;
//		virtual SizeMap getCumulativeUploadSizesBytes(const Key& driveKey, const Key& replicatorKey) = 0;

        virtual std::unique_ptr<ApprovedDataModification> getLastApprovedDataModification(const Key& driveKey) = 0;

		virtual uint64_t getDownloadWorkBytes(const Key& replicatorKey, const Key& driveKey) = 0;

		virtual bool downloadChannelExists(const Hash256& id) = 0;
		virtual std::unique_ptr<DownloadChannel> getDownloadChannel(const Key& replicatorKey, const Hash256& id) = 0;

		virtual std::optional<DriveVerification> getActiveVerification(const Key& driveKey, const Timestamp& blockTimestamp) = 0;

	protected:
		cache::CatapultCache* m_pCache;
		model::BlockElementSupplier m_lastBlockElementSupplier;
	};
}}
