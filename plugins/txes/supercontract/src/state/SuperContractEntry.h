/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/types.h"
#include "catapult/exceptions.h"
#include "catapult/model/Mosaic.h"
#include "catapult/utils/ArraySet.h"
#include "catapult/utils/IntegerMath.h"
#include <map>

namespace catapult { namespace state {

    /// Super contract state.
    enum class SuperContractState : uint8_t {
        /// Super contract is active.
        Active,

        /// Super contract is deactivated by owner or executors.
        DeactivatedByParticipant,

        /// Super contract is deactivated by drive end.
        DeactivatedByDriveEnd,
    };

//
//	struct ExecutorInfo {
//		Height Start;
//		Height End;
//	};
//
//	/// The map where key is executor and value is info.
//	using ExecutorsMap = std::map<Key, ExecutorInfo>;

	// Mixin for storing super contract details.
	class SuperContractMixin {
	public:
		SuperContractMixin()
			: m_state(SuperContractState::Active)
			, m_executionCount(0)
		{}

	public:
		/// Sets \a state of super contract.
		void setState(SuperContractState state) {
			m_state = state;
		}

		/// Gets state of super contract.
		SuperContractState state() const {
			return m_state;
		}

        /// Sets \a owner of super contract.
        void setOwner(const Key& owner) {
            m_owner = owner;
        }

        /// Gets owner of super contract.
        const Key& owner() const {
            return m_owner;
        }

		/// Sets start \a height of super contract.
		void setStart(const Height& height) {
			m_start = height;
		}

		/// Gets start height of super contract.
		const Height& start() const {
			return m_start;
		}

		/// Sets end \a height of super contract.
		void setEnd(const Height& height) {
			m_end = height;
		}

		/// Gets end height of super contract.
		const Height& end() const {
			return m_end;
		}

		/// Sets VM \a version.
		void setVmVersion(const VmVersion & version) {
			m_vmVersion = version;
		}

		/// Gets VM version.
		const VmVersion& vmVersion() const {
			return m_vmVersion;
		}

        /// Sets \a key of according main drive.
        void setMainDriveKey(const Key& key) {
	        m_mainDriveKey = key;
        }

        /// Gets key of according main drive.
        const Key& mainDriveKey() const {
            return m_mainDriveKey;
        }

        /// Sets \a file hash of super contract.
        void setFileHash(const Hash256& fileHash) {
	        m_fileHash = fileHash;
        }

        /// Gets file hash of super contract.
        const Hash256& fileHash() const {
            return m_fileHash;
        }

        /// Sets the \a executionCount.
        void setExecutionCount(const uint16_t& executionCount) {
            m_executionCount = executionCount;
        }

        /// Increments the execution count.
        void incrementExecutionCount() {
			if (std::numeric_limits<uint16_t>::max() == m_executionCount)
				CATAPULT_THROW_RUNTIME_ERROR("failed to increment execution count, limit reached");
            m_executionCount++;
        }

        /// Decrements the execution count.
        void decrementExecutionCount() {
			if (0 == m_executionCount)
				CATAPULT_THROW_RUNTIME_ERROR("failed to decrement execution count below zero");
            m_executionCount--;
        }

        /// Gets the execution count.
        const uint16_t& executionCount() const {
            return m_executionCount;
        }
//
//		/// Gets executor infos.
//		const ExecutorsMap& executors() const {
//			return m_executors;
//		}
//
//		/// Gets executor infos.
//		ExecutorsMap& executors() {
//			return m_executors;
//		}
//
//		/// Gets the array of additional drives of super contract.
//		const std::vector<Key>& additionalDrives() const {
//			return m_additionalDrives;
//		}
//
//		/// Gets the array of additional drives of super contract.
//		std::vector<Key>& additionalDrives() {
//			return m_additionalDrives;
//		}

	private:
		SuperContractState m_state;
		Key m_owner;
		Height m_start;
		Height m_end;
		VmVersion m_vmVersion;
		Key m_mainDriveKey;
		Hash256 m_fileHash;
		uint16_t m_executionCount;
//		// TODO: In future we plan to have external executors
//		ExecutorsMap m_executors;
//		// TODO: In future we plan to support several drives for one super contract
//		std::vector<Key> m_additionalDrives;
	};

	// SuperContract entry.
	class SuperContractEntry : public SuperContractMixin {
	public:
		// Creates a super contract entry around \a key.
		explicit SuperContractEntry(const Key& key) : m_key(key)
		{}

	public:
		// Gets the super contract public key.
		const Key& key() const {
			return m_key;
		}

	private:
		Key m_key;
	};
}}
