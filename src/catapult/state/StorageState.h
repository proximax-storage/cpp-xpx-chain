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
#include <optional>

namespace catapult { namespace cache { class CatapultCache; } }

namespace catapult { namespace state {

	/// Data modification state.
	enum class DataModificationApprovalState : uint8_t {
		/// Data modification has been approved.
		Approved,

		/// Data modification has been cancelled.
		Cancelled
	};

	struct DataModification {
		Hash256 Id;
		Key DriveKey;
		Hash256 DownloadDataCdi;
		uint64_t ExpectedUploadSize;
		uint64_t ActualUploadSize;
		std::string FolderName;
		bool ReadyForApproval;
		bool IsStream;
	};

	struct ApprovedDataModification : DataModification {
		std::vector<Key> Signers;
	};

	struct DownloadChannel {
		Hash256 Id;
		Key DriveKey;
		uint64_t DownloadSizeMegabytes;
		std::vector<Key> Consumers;
		utils::KeySet Replicators;
		std::optional<Hash256> ApprovalTrigger;
	};

	struct DriveVerification {
		Key DriveKey;
		uint32_t Duration;
		bool Expired;
		Hash256 VerificationTrigger;
		Hash256 ModificationId;
		std::vector<std::set<Key>> Shards;
	};

	struct ModificationShard {
		std::map<Key, uint64_t> ActualShardMembers;
		std::map<Key, uint64_t> FormerShardMembers;
		uint64_t OwnerUpload = 0;
	};

	struct CompletedModification {
		Hash256 ModificationId;
		DataModificationApprovalState Status;
	};

	struct ReplicatorDriveInfo {
		/// Identifier of the most recent data modification of the drive approved by the replicator.
		Hash256 LastApprovedDataModificationId;

		/// Used drive size at the time of the replicator’s onboarding excluding metafiles size in megabytes.
		/// Set to \p 0 after replicator’s first data modification approval.
		uint64_t InitialDownloadWorkMegabytes;

		/// Size of cumulative download work
		uint64_t LastCompletedCumulativeDownloadWorkBytes;
	};

	struct Drive {
		Key Id;
		Key Owner;
		Hash256 RootHash;
		uint64_t Size;
		utils::KeySet Replicators;
		std::unordered_map<Key, ReplicatorDriveInfo, utils::ArrayHasher<Key>> ReplicatorInfo;
		std::unordered_map<Key, ModificationShard, utils::ArrayHasher<Key>> ModificationShards;
		std::vector<Key> DonatorShard;
		std::vector<Key> RecipientShard;
		std::unordered_map<Hash256, std::shared_ptr<DownloadChannel>, utils::ArrayHasher<Hash256>> DownloadChannels;
		std::vector<DataModification> DataModifications;
		std::vector<CompletedModification> CompletedModifications;
		uint64_t DownloadWorkBytes;
		std::shared_ptr<ApprovedDataModification> LastApprovedDataModificationPtr;
		std::shared_ptr<DriveVerification> ActiveVerificationPtr;
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

		void setReplicatorKey(const Key& replicatorKey) {
			m_replicatorKey = replicatorKey;
		}

		const Key& replicatorKey() const {
			return m_replicatorKey;
		}

	public:
		virtual bool isReplicatorRegistered() = 0;
		virtual std::shared_ptr<Drive> getDrive(const Key& driveKey, const Timestamp& timestamp) = 0;
		virtual std::vector<std::shared_ptr<Drive>> getDrives(const Timestamp& timestamp) = 0;

	protected:
		cache::CatapultCache* m_pCache;
		model::BlockElementSupplier m_lastBlockElementSupplier;
		Key m_replicatorKey;
	};
}}
