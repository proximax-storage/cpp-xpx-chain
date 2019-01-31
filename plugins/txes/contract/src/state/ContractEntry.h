/**
*** Copyright 2018 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/types.h"
#include "catapult/utils/ArraySet.h"
#include "src/model/HashSnapshot.h"
#include <vector>

namespace catapult { namespace state {

	// Mixin for storing contract details.
	class ContractMixin {
	public:
		/// Gets the start height of the contract.
		Height start() const {
			return m_start;
		}

		/// Sets the \a start height of the contract.
		void setStart(const Height& start) {
			m_start = start;
		}

		/// Gets the duration of the contract in blocks.
		BlockDuration duration() const {
			return m_duration;
		}

		/// Sets the \a duration of the contract in blocks.
		void setDuration(const BlockDuration& duration) {
			m_duration = duration;
		}

		/// Gets the last hash of an entity passed from customers to executors (e.g. file hash).
		Hash256 hash() const {
			return m_hashes.empty() ? Hash256() : m_hashes.back().Hash;
		}

		/// Gets the hashes of an entity passed from customers to executors (e.g. file hash).
		const std::vector<model::HashSnapshot>& hashes() const {
			return m_hashes;
		}

		/// Pushs the \a hash of an entity passed from customers to executors (e.g. file hash) at \a height to array of hashes.
		void pushHash(const Hash256 & hash, const Height& height) {
			m_hashes.emplace_back(model::HashSnapshot{ hash, height });
		}

		/// Pops the last \a hash of an entity passed from customers to executors (e.g. file hash) from array of hashes.
		void popHash() {
			m_hashes.pop_back();
		}

		/// Gets customer account keys.
		const utils::SortedKeySet& customers() const {
			return m_customers;
		}

		/// Gets customer account keys.
		utils::SortedKeySet& customers() {
			return m_customers;
		}

		/// Returns \c true if \a key is a customer.
		bool hasCustomer(const Key& key) const {
			return m_customers.end() != m_customers.find(key);
		}

		/// Gets executor account keys.
		const utils::SortedKeySet& executors() const {
			return m_executors;
		}

		/// Gets executor account keys.
		utils::SortedKeySet& executors() {
			return m_executors;
		}

		/// Returns \c true if \a key is a executor.
		bool hasExecutor(const Key& key) const {
			return m_executors.end() != m_executors.find(key);
		}

		/// Gets verifier account keys.
		const utils::SortedKeySet& verifiers() const {
			return m_verifiers;
		}

		/// Gets verifier account keys.
		utils::SortedKeySet& verifiers() {
			return m_verifiers;
		}

		/// Returns \c true if \a key is a verifier.
		bool hasVerifier(const Key& key) const {
			return m_verifiers.end() != m_verifiers.find(key);
		}

	private:
		Height m_start;
		BlockDuration m_duration;
		std::vector<model::HashSnapshot> m_hashes;
		utils::SortedKeySet m_customers;
		utils::SortedKeySet m_executors;
		utils::SortedKeySet m_verifiers;
	};

	// Contract entry.
	class ContractEntry : public ContractMixin {
	public:
		// Creates a contract entry around \a key.
		explicit ContractEntry(const Key& key) : m_key(key)
		{}

	public:
		// Gets the account public key.
		const Key& key() const {
			return m_key;
		}

	private:
		Key m_key;
	};
}}
