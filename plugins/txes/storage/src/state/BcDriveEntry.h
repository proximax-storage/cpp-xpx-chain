/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/types.h"
#include "catapult/exceptions.h"
#include "catapult/model/Mosaic.h"
#include "catapult/utils/ArraySet.h"
#include "catapult/utils/IntegerMath.h"
#include "CommonEntities.h"
#include <vector>
#include <optional>

namespace catapult { namespace state {

	/// Data modification state.
	enum class DataModificationState : uint8_t {
		/// Data modification has been approved.
		Succeeded,

		/// Data modification has been cancelled.
		Cancelled
	};

	struct ActiveDataModification {

		/// Constructor For Data Modification
		ActiveDataModification(
				const Hash256& id,
				const Key& owner,
				const Hash256& downloadDataCdi,
				const uint64_t& uploadSizeMegabytes)
				: ActiveDataModification(id, owner, downloadDataCdi, uploadSizeMegabytes, uploadSizeMegabytes, "", true)
		{}

		/// Constructor For Stream Start
		ActiveDataModification(
				const Hash256& id,
				const Key& owner,
				const uint64_t& expectedUploadSizeMegabytes,
				const std::string& folderName)
			: ActiveDataModification(id, owner, Hash256(),
					  expectedUploadSizeMegabytes,
					  expectedUploadSizeMegabytes, folderName, false)
		{}

		ActiveDataModification(
				const Hash256& id,
				const Key& owner,
				const Hash256& downloadDataCdi,
				const uint64_t& expectedUploadSizeMegabytes,
				const uint64_t& actualUploadSizeMegabytes,
				const std::string& folderName,
				const bool& readyForApproval)
			: Id(id)
			, Owner(owner)
			, DownloadDataCdi(downloadDataCdi)
			, ExpectedUploadSizeMegabytes(expectedUploadSizeMegabytes)
			, ActualUploadSizeMegabytes(actualUploadSizeMegabytes)
			, FolderName(folderName)
			, ReadyForApproval(readyForApproval)
		{}

		/// Id of data modification.
		Hash256 Id;

		/// Public key of the drive owner.
		Key Owner;

		/// CDI of download data. Zero for Stream
		Hash256 DownloadDataCdi;

		/// Expected Upload size of data.
		uint64_t ExpectedUploadSizeMegabytes;

		/// Actual Upload size of data. Differs from ExpectedUploadSize only for streams
		uint64_t ActualUploadSizeMegabytes;

		/// FolderName for stream
		std::string FolderName;

		/// Whether DataModification can be approved by Replicators
		bool ReadyForApproval;
	};

	struct CompletedDataModification : ActiveDataModification {
		CompletedDataModification(const ActiveDataModification& modification, DataModificationState state)
			: ActiveDataModification(modification)
			, State(state)
		{}

		/// Completion state.
		DataModificationState State;
	};

	struct ConfirmedStorageInfo {
		Timestamp TimeInConfirmedStorage = Timestamp(0);
		std::optional<Timestamp> ConfirmedStorageSince;
	};

	struct ModificationShardInfo {
		std::map<Key, uint64_t> ActualShardMembers;
		std::map<Key, uint64_t> FormerShardMembers;
		uint64_t OwnerUpload = 0;

		std::set<Key> getActualShardMembersKeys() {
			std::set<Key> keys;
			for (const auto& [key, _]: ActualShardMembers) {
				keys.insert(key);
			}
		};
	};

	using ActiveDataModifications = std::vector<ActiveDataModification>;
	using CompletedDataModifications = std::vector<CompletedDataModification>;
	using SizeMap = std::map<Key, uint64_t>;
	using ConfirmedStates = std::map<Key, Hash256>; // last approved root hash
	using ConfirmedStorageInfos = std::map<Key, ConfirmedStorageInfo>;
	using Shards = std::vector<std::set<Key>>;
	using DownloadShards = std::set<Hash256>;
	using ModificationShards = std::map<Key, ModificationShardInfo>;

	struct Verification {
		/// The hash of block that initiated the verification.
		Hash256 VerificationTrigger;

		/// The expiration time of the verification.
		Timestamp Expiration;

		/// The duration of the verification in ms
		uint32_t Duration;

		/// Replicator shards.
		state::Shards Shards;

		bool expired(const Timestamp& timestamp) const {
			return timestamp >= Expiration;
		}
	};

	// Mixin for storing drive details.
	class DriveMixin {
	public:
		DriveMixin()
			: m_size(0)
			, m_usedSizeBytes(0)
			, m_metaFilesSizeBytes(0)
			, m_replicatorCount(0)
		{}

	public:
		/// Sets \a owner of drive.
		void setOwner(const Key& owner) {
			m_owner = owner;
		}

		/// Gets owner of drive.
		const Key& owner() const {
			return m_owner;
		}

		/// Sets \a rootHash of drive.
		void setRootHash(const Hash256& rootHash) {
			m_rootHash = rootHash;
		}

		/// Gets root hash of drive.
		const Hash256& rootHash() const {
			return m_rootHash;
		}

		/// Sets total size of the drive.
		void setSize(const uint64_t& size) {
			m_size = size;
		}

		/// Gets total size of the drive.
		const uint64_t& size() const {
			return m_size;
		}

		/// Sets used size of the drive.
		void setUsedSizeBytes(const uint64_t& usedSize) {
			m_usedSizeBytes = usedSize;
		}

		/// Gets used size of the drive.
		const uint64_t& usedSizeBytes() const {
			return m_usedSizeBytes;
		}

		/// Sets the size of the drive metafiles.
		void setMetaFilesSizeBytes(const uint64_t& metaFilesSize) {
			m_metaFilesSizeBytes = metaFilesSize;
		}

		/// Gets the size of the drive metafiles.
		const uint64_t& metaFilesSizeBytes() const {
			return m_metaFilesSizeBytes;
		}

		/// Sets the number of the drive \a replicas.
		void setReplicatorCount(uint16_t replicatorCount) {
			m_replicatorCount = replicatorCount;
		}

		/// Gets the number of the drive replicas.
		const uint16_t& replicatorCount() const {
			return m_replicatorCount;
		}

		/// Gets active data modifications.
		const ActiveDataModifications& activeDataModifications() const {
			return m_activeDataModifications;
		}

		/// Gets active data modifications.
		ActiveDataModifications& activeDataModifications() {
			return m_activeDataModifications;
		}

		/// Gets finished data modifications.
		const CompletedDataModifications& completedDataModifications() const {
			return m_completedDataModifications;
		}

		/// Gets finished data modifications.
		CompletedDataModifications& completedDataModifications() {
			return m_completedDataModifications;
		}

		/// Gets map with key replicator public key and value used drive size.
		const SizeMap& confirmedUsedSizes() const {
			return m_confirmedUsedSizeMap;
		}

		/// Gets infos of drives assigned to the replicator.
		SizeMap& confirmedUsedSizes() {
			return m_confirmedUsedSizeMap;
		}

		/// Gets replicators.
		const utils::SortedKeySet& replicators() const {
			return m_replicators;
		}

		/// Gets replicators.
		utils::SortedKeySet& replicators() {
			return m_replicators;
		}

		/// Gets former replicators.
		const utils::SortedKeySet& formerReplicators() const {
			return m_formerReplicators;
		}

		/// Gets former replicators.
		utils::SortedKeySet& formerReplicators() {
			return m_formerReplicators;
		}

		/// Gets replicators that applied for offboarding.
		/// Must be a subset of \a m_replicators.
		const std::vector<Key>& offboardingReplicators() const {
			return m_offboardingReplicators;
		}

		/// Gets replicators that applied for offboarding.
		/// Must be a subset of \a m_replicators.
		std::vector<Key>& offboardingReplicators() {
			return m_offboardingReplicators;
		}

		/// Gets verifications.
		std::optional<Verification>& verification() {
			return m_verification;
		}

		/// Gets verifications.
		const std::optional<Verification>& verification() const {
			return m_verification;
		}

		/// Gets replicators last confirmed states.
		const ConfirmedStates& confirmedStates() const {
			return m_confirmedStates;
		}

		/// Gets replicators last confirmed states.
		ConfirmedStates& confirmedStates() {
			return m_confirmedStates;
		}

		/// Gets replicators last confirmed storage infos.
		const ConfirmedStorageInfos& confirmedStorageInfos() const {
			return m_confirmedStoragePeriods;
		}

		/// Gets replicators last confirmed storage infos.
		ConfirmedStorageInfos& confirmedStorageInfos() {
			return m_confirmedStoragePeriods;
		}

		/// Gets map of download channels and replicators that belong to respective download shards.
		const DownloadShards& downloadShards() const {
			return m_downloadShards;
		}

		/// Gets map of download channels and replicators that belong to respective download shards.
		DownloadShards& downloadShards() {
			return m_downloadShards;
		}

		/// Gets map of data modification shards. The first set in pair represents replicator's current shard,
		/// and the second set contains additional keys on which this replicator is allowed to provide opinion as well.
		const ModificationShards& dataModificationShards() const {
			return m_dataModificationShards;
		}

		/// Gets map of data modification shards. The first set in pair represents replicator's current shard,
		/// and the second set contains additional keys on which this replicator is allowed to provide opinion as well.
		ModificationShards& dataModificationShards() {
			return m_dataModificationShards;
		}

		const Key& getQueueNext() const {
			return m_storagePaymentsQueueNext;
		}

		void setQueueNext(const Key& storagePaymentsQueueNext) {
			m_storagePaymentsQueueNext = storagePaymentsQueueNext;
		}

		const Key& getQueuePrevious() const {
			return m_storagePaymentsQueuePrevious;
		}

		void setQueuePrevious(const Key& storagePaymentsQueuePrevious) {
			m_storagePaymentsQueuePrevious = storagePaymentsQueuePrevious;
		}

		const Timestamp& getLastPayment() const {
			return m_lastPayment;
		}

		void setLastPayment(const Timestamp& lastPayment) {
			m_lastPayment = lastPayment;
		}

		const AVLTreeNode& verificationNode() const {
			return m_verificationNode;
		}

		AVLTreeNode& verificationNode() {
			return m_verificationNode;
		}

	private:
		Key m_owner;
		Hash256 m_rootHash;
		uint64_t m_size;
		uint64_t m_usedSizeBytes;
		uint64_t m_metaFilesSizeBytes;
		uint16_t m_replicatorCount;
		ActiveDataModifications m_activeDataModifications;
		CompletedDataModifications m_completedDataModifications;
		SizeMap m_confirmedUsedSizeMap;
		utils::SortedKeySet m_replicators;
		utils::SortedKeySet m_formerReplicators;
		std::vector<Key> m_offboardingReplicators;
		std::optional<Verification> m_verification;
		ConfirmedStates m_confirmedStates;
		ConfirmedStorageInfos m_confirmedStoragePeriods;
		DownloadShards m_downloadShards;
		ModificationShards m_dataModificationShards;
		Key m_storagePaymentsQueueNext;
		Key m_storagePaymentsQueuePrevious;
		Timestamp m_lastPayment;

		AVLTreeNode m_verificationNode;
	};

	// Drive entry.
	class BcDriveEntry : public DriveMixin {
	public:
		// Creates a drive entry around \a key.
		explicit BcDriveEntry(const Key& key) : m_key(key), m_version(1)
		{}

	public:
		// Gets the drive public key.
		const Key& key() const {
			return m_key;
		}

		Key entryKey() {
			return m_key;
		}

		void setVersion(VersionType version) {
			m_version = version;
		}

		VersionType version() const {
			return m_version;
		}

	private:
		Key m_key;
		VersionType m_version;
	};
}}
