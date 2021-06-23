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

	// Mixin for storing drive details.
	class DriveMixin {
	public:
		DriveMixin()
			: m_size(0)
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

		/// Sets the drive size.
		void setSize(const uint64_t& size) {
			m_size = size;
		}

		/// Gets the drive size.
		const uint64_t& size() const {
			return m_size;
		}

		/// Sets the number of the drive \a replicas.
		void setReplicatorCount(uint16_t replicatorCount) {
			m_replicatorCount = replicatorCount;
		}

		/// Gets the number of the drive replicas.
		const uint16_t& replicatorCount() const {
			return m_replicatorCount;
		}

		/// Sets \a verificationFeeAmount of the drive.
		void setVerificationFeeAmount(const Amount& verificationFeeAmount) {
			m_verificationFeeAmount = verificationFeeAmount;
		}

		/// Gets verification fee amount.
		const Amount& verificationFeeAmount() const {
			return m_verificationFeeAmount;
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

	private:
		Key m_owner;
		Hash256 m_rootHash;
		uint64_t m_size;
		uint16_t m_replicatorCount;
		Amount m_verificationFeeAmount;
		ActiveDataModifications m_activeDataModifications;
		CompletedDataModifications m_completedDataModifications;
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
