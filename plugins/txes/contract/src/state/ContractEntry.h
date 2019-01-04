/**
*** Copyright (c) 2018-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#pragma once
#include "catapult/types.h"
#include "catapult/utils/ArraySet.h"

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

		/// Gets the hash of an entity passed from customers to executors (e.g. file hash).
		Hash256 hash() const {
			return m_hash;
		}

		/// Sets the \a hash of an entity passed from customers to executors (e.g. file hash).
		void setHash(const Hash256& hash) {
			m_hash = hash;
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
		Hash256 m_hash;
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
