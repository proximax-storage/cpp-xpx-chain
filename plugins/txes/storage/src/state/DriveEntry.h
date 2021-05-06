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
#include <algorithm>

namespace catapult { namespace state {

	/// Data modification state.
	enum class DataModificationState : uint8_t {
		/// Default data modification state.	// TODO: Rephrase
		None,

		/// Data modification is queued.
		Queued,

		/// Data modification is waiting for approval.
		Active
	};

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

		/// Sets the drive size.
		void setSize(const Amount& size) {
			m_size = size;
		}

		/// Gets the drive size.
		const Amount& size() const {
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

		/// Gets the queue of data modification hashes.
		const std::vector<std::pair<Hash256, DataModificationState>>& dataModificationQueue() const {
			return m_dataModificationQueue;
		}

		/// Gets the queue of data modification hashes.
		std::vector<std::pair<Hash256, DataModificationState>>& dataModificationQueue() {
			return m_dataModificationQueue;
		}

	private:
		Key m_owner;
		Amount m_size;
		uint16_t m_replicatorCount;
		std::vector<std::pair<Hash256, DataModificationState>> m_dataModificationQueue;
	};

	// Drive entry.
	class DriveEntry : public DriveMixin {
	public:
		// Creates a drive entry around \a key.
		explicit DriveEntry(const Key& key) : m_key(key), m_version(1)
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
