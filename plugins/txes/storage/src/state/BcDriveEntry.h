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
		/// Id of data modification.
		Hash256 Id;

		/// Public key of the drive owner.
		Key Owner;

		/// CDI of download data.
		Hash256 DownloadDataCdi;

		/// Upload size of data.
		uint64_t UploadSize;
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

	using ConfirmedStates = std::map<Key, Hash256>; // last approved root hash

    /// Verification State.
    enum class VerificationState : uint8_t {
        /// Verification waits for opinions.
        Pending,

        /// Verification was canceled. For example by DataModificationApprovalTransaction.
		Canceled,

        /// Verification finished.
        Finished
    };

    using VerificationOpinions = std::map<Key, uint8_t>;

	struct Verification {
	    /// The hash of block that initiated the Verification.
	    Hash256 VerificationTrigger;

        /// Verification opinions.
        VerificationOpinions Opinions;

		/// State of verification.
		VerificationState State;
	};

	using Verifications = std::vector<Verification>;

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

		/// Gets verifications.
		Verifications& verifications() {
			return m_verifications;
		}

		/// Gets verifications.
		const Verifications& verifications() const {
			return m_verifications;
		}

		/// Gets replicators last confirmed states.
		const ConfirmedStates& confirmedStates() const {
			return m_confirmedStates;
		}

		/// Gets replicators last confirmed states.
		ConfirmedStates& confirmedStates() {
			return m_confirmedStates;
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
		Verifications m_verifications;
		ConfirmedStates m_confirmedStates;
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
