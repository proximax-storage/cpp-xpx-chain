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
#include <vector>

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
				const uint64_t& uploadSize)
			: ActiveDataModification(id, owner, downloadDataCdi, uploadSize, uploadSize, "", true)
		{}

		/// Constructor For Stream Start
		ActiveDataModification(
				const Hash256& id,
				const Key& owner,
				const uint64_t& expectedUploadSize,
				const std::string& folderName)
			: ActiveDataModification(id, owner, Hash256(), expectedUploadSize, expectedUploadSize, folderName, false)
		{}

		ActiveDataModification(
				const Hash256& id,
				const Key& owner,
				const Hash256& downloadDataCdi,
				const uint64_t& expectedUploadSize,
				const uint64_t& actualUploadSize,
				const std::string& folderName,
				const bool& readyForApproval)
			: Id(id)
			, Owner(owner)
			, DownloadDataCdi(downloadDataCdi)
			, ExpectedUploadSize(expectedUploadSize)
			, ActualUploadSize(actualUploadSize)
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
		uint64_t ExpectedUploadSize;

		/// Actual Upload size of data. Differs from ExpectedUploadSize only for streams
		uint64_t ActualUploadSize;

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

	using ActiveDataModifications = std::vector<ActiveDataModification>;
	using CompletedDataModifications = std::vector<CompletedDataModification>;
	/// The map where key is replicator public key and value is last used drive size approved by the replicator.
	using UsedSizeMap = std::map<Key, u_int64_t>;

	// Mixin for storing drive details.
	class DriveMixin {
	public:
		DriveMixin()
			: m_size(0)
			, m_usedSize(0)
			, m_metaFilesSize(0)
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
		void setUsedSize(const uint64_t& usedSize) {
			m_usedSize = usedSize;
		}

		/// Gets used size of the drive.
		const uint64_t& usedSize() const {
			return m_usedSize;
		}

		/// Sets the size of the drive metafiles.
		void setMetaFilesSize(const uint64_t& metaFilesSize) {
			m_metaFilesSize = metaFilesSize;
		}

		/// Gets the size of the drive metafiles.
		const uint64_t& metaFilesSize() const {
			return m_metaFilesSize;
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
		const UsedSizeMap& confirmedUsedSizes() const {
			return m_usedSizeMap;
		}

		/// Gets infos of drives assigned to the replicator.
		UsedSizeMap& confirmedUsedSizes() {
			return m_usedSizeMap;
		}

		/// Gets replicators.
		const utils::KeySet& replicators() const {
			return m_replicators;
		}

		/// Gets replicators.
		utils::KeySet& replicators() {
			return m_replicators;
		}

	private:
		Key m_owner;
		Hash256 m_rootHash;
		uint64_t m_size;
		uint64_t m_usedSize;
		uint64_t m_metaFilesSize;
		uint16_t m_replicatorCount;
		ActiveDataModifications m_activeDataModifications;
		CompletedDataModifications m_completedDataModifications;
		UsedSizeMap m_usedSizeMap;
		utils::KeySet m_replicators;
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
