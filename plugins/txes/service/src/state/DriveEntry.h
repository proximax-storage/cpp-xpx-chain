/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/types.h"
#include "catapult/model/Mosaic.h"
#include "catapult/utils/ArraySet.h"
#include <map>

namespace catapult { namespace state {

	struct FileInfo {
		Height Start;
        Amount Deposit;
		uint64_t Size;

		/// Index of array is index of replicator, the value is height of deposit
		std::vector<Height> ReplicatorsDepositHeight;
	};

	/// The map where key is hash of the file and value is file info.
	using FilesMap = std::map<Hash256, FileInfo, utils::ArrayHasher<Hash256>>;

	struct ReplicatorInfo {
		Height Start;
		Amount Deposit;
	};

	/// The map where key is replicator and value is info.
	using ReplicatorsMap = std::map<Key, ReplicatorInfo, utils::ArrayHasher<Key>>;

	// Mixin for storing drive details.
	class DriveMixin {
	public:
		/// Gets the start height of the drive.
		Height start() const {
			return m_start;
		}

		/// Sets the \a start height of the drive.
		void setStart(const Height& start) {
			m_start = start;
		}

		/// Gets the duration of the drive in blocks.
		BlockDuration duration() const {
			return m_duration;
		}

		/// Sets the \a duration of the drive in blocks.
		void setDuration(const BlockDuration& duration) {
			m_duration = duration;
		}

		/// Gets the drive size.
		uint64_t size() const {
			return m_size;
		}

		/// Gets the drive deposit.
		const Amount& deposit() const {
			return m_deposit;
		}

		/// Gets the drive deposit.
		Amount& deposit() {
			return m_deposit;
		}

		/// Sets the drive \a size.
		void setSize(uint64_t size) {
			m_size = size;
		}

		/// Gets the number of the drive replicas.
		uint8_t replicas() const {
			return m_replicas;
		}

		/// Sets the number of the drive \a replicas.
		void setReplicas(uint8_t replicas) {
			m_replicas = replicas;
		}

		/// Gets the number of the drive replicas.
		uint8_t minReplicators() const {
			return m_minReplicators;
		}

		/// Sets the number of the drive \a replicators which need for start of drive.
		void setMinReplicators(uint8_t replicators) {
			m_minReplicators = replicators;
		}

		/// Gets the number of the drive replicators for starting.
		uint8_t minApprovers() const {
			return m_minApprovers;
		}

		/// Sets the number of the drive \a approvers.
		void setMinApprovers(uint8_t approvers) {
			m_minApprovers = approvers;
		}

		/// Gets files.
		const FilesMap& files() const {
			return m_files;
		}

		/// Gets files.
		FilesMap& files() {
			return m_files;
		}

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

		/// Returns \c true if drive contains a file with \a hash.
		bool hasFile(const Hash256& hash) const {
			return m_files.end() != m_files.find(hash);
		}

		/// Gets replicator account keys.
		const ReplicatorsMap& replicators() const {
			return m_replicators;
		}

		/// Gets replicator account keys.
		ReplicatorsMap& replicators() {
			return m_replicators;
		}

		/// Returns \c true if \a key is a replicator.
		bool hasReplicator(const Key& key) const {
			return m_replicators.end() != m_replicators.find(key);
		}

	private:
		Height m_start;
		BlockDuration m_duration;
		uint64_t m_size;
		Amount m_deposit;
		uint8_t m_replicas;
		uint8_t m_minReplicators;
		uint8_t m_minApprovers;
		Key m_owner;
		Hash256 m_rootHash;
		FilesMap m_files;
		ReplicatorsMap m_replicators;
	};

	// Drive entry.
	class DriveEntry : public DriveMixin {
	public:
		// Creates a drive entry around \a key.
		explicit DriveEntry(const Key& key) : m_key(key)
		{}

	public:
		// Gets the drive public key.
		const Key& key() const {
			return m_key;
		}

	private:
		Key m_key;
	};
}}
