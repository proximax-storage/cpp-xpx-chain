/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/types.h"
#include "catapult/model/Mosaic.h"
#include "catapult/utils/ArraySet.h"
#include <vector>

namespace catapult { namespace state {

	/// The map where key is hash of the deposited file and value is deposit mosaic.
	/// Empty hash if the deposit is made for the drive.
	using DepositMap = std::unordered_map<Hash256, model::UnresolvedMosaic, utils::ArrayHasher<Hash256>>;
	using ContractorDepositMap = std::unordered_map<Key, DepositMap, utils::ArrayHasher<Key>>;

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

		/// Sets the drive \a size.
		void setSize(uint64_t size) {
			m_size = size;
		}

		/// Gets the number of the drive replicas.
		uint16_t replicas() const {
			return m_replicas;
		}

		/// Sets the number of the drive \a replicas.
		void setReplicas(uint16_t replicas) {
			m_replicas = replicas;
		}

		/// Gets customer account keys.
		const ContractorDepositMap& customers() const {
			return m_customers;
		}

		/// Gets customer account keys.
		ContractorDepositMap& customers() {
			return m_customers;
		}

		/// Returns \c true if \a key is a customer.
		bool hasCustomer(const Key& key) const {
			return m_customers.end() != m_customers.find(key);
		}

		/// Gets replicator account keys.
		const ContractorDepositMap& replicators() const {
			return m_replicators;
		}

		/// Gets replicator account keys.
		ContractorDepositMap& replicators() {
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
		uint16_t m_replicas;
		ContractorDepositMap m_customers;
		ContractorDepositMap m_replicators;
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
