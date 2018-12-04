/**
*** Copyright (c) 2016-present,
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
#include "catapult/utils/ArraySet.h"

namespace catapult { namespace state {

	/// Mixin for storing information about cosignatories of an account.
	class MultisigCosignatoriesMixin {
	public:
		/// Creates multisig cosignatories mixin.
		MultisigCosignatoriesMixin()
				: m_minApproval(0)
				, m_minRemoval(0)
		{}

	public:
		/// Gets cosignatory account keys.
		const utils::SortedKeySet& cosignatories() const {
			return m_cosignatories;
		}

		/// Gets cosignatory account keys.
		utils::SortedKeySet& cosignatories() {
			return m_cosignatories;
		}

		/// Returns \c true if \a key is a cosignatory.
		bool hasCosignatory(const Key& key) const {
			return m_cosignatories.end() != m_cosignatories.find(key);
		}

		/// Gets the number of cosignatories required when approving (any) transaction.
		uint8_t minApproval() const {
			return m_minApproval;
		}

		/// Sets the number of cosignatories required (\a minApproval) when approving (any) transaction.
		void setMinApproval(uint8_t minApproval) {
			m_minApproval = minApproval;
		}

		/// Gets the number of cosignatories required when removing an account.
		uint8_t minRemoval() const {
			return m_minRemoval;
		}

		/// Sets the number of cosignatories required (\a minRemoval) when removing an account.
		void setMinRemoval(uint8_t minRemoval) {
			m_minRemoval = minRemoval;
		}

	private:
		utils::SortedKeySet m_cosignatories;
		uint8_t m_minApproval;
		uint8_t m_minRemoval;
	};

	/// Mixin for storing information about accounts that current account can cosign.
	class MultisigCosignatoryOfMixin {
	public:
		/// Gets multisig account keys.
		const utils::SortedKeySet& multisigAccounts() const {
			return m_multisigAccounts;
		}

		/// Gets multisig account keys.
		utils::SortedKeySet& multisigAccounts() {
			return m_multisigAccounts;
		}

	private:
		utils::SortedKeySet m_multisigAccounts;
	};

	/// Multisig entry.
	class MultisigEntry : public MultisigCosignatoriesMixin, public MultisigCosignatoryOfMixin {
	public:
		/// Creates a multisig entry around \a key.
		explicit MultisigEntry(const Key& key) : m_key(key)
		{}

	public:
		/// Gets the account public key.
		const Key& key() const {
			return m_key;
		}

	private:
		Key m_key;
	};
}}
