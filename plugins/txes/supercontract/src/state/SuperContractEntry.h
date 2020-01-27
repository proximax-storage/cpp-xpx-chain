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
		SuperContractMixin() = default;

	public:
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

		/// Sets \a version of super contract.
		void setVersion(const BlockchainVersion & version) {
			m_version = version;
		}

		/// Gets end height of super contract.
		const BlockchainVersion& version() const {
			return m_version;
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
		Height m_start;
		Height m_end;
		BlockchainVersion m_version;
		Key m_mainDriveKey;
		Hash256 m_fileHash;
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
