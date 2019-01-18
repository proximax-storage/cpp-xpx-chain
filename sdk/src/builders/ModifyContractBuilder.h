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
#include "TransactionBuilder.h"
#include "plugins/txes/contract/src/model/ModifyContractTransaction.h"
#include <vector>

namespace catapult { namespace builders {

	/// Builder for a modify contract transaction.
	class ModifyContractBuilder : public TransactionBuilder {
	public:
		using Transaction = model::ModifyContractTransaction;
		using EmbeddedTransaction = model::EmbeddedModifyContractTransaction;

		/// Creates a modify contract builder for building a modify contract transaction from \a signer
		/// for the network specified by \a networkIdentifier.
		ModifyContractBuilder(model::NetworkIdentifier networkIdentifier, const Key& signer);

	public:
		/// Sets the \a durationDelta of the contract.
		void setDurationDelta(const int64_t& durationDelta);

		/// Sets the \a multisig account of the contract.
		void setMultisig(const Key& multisig);

		/// Sets the \a hash of an entity passed from customers to executors (e.g. file hash).
		void setHash(const Hash256& hash);

		/// Adds a customer modification around \a type and \a key.
		void addCustomerModification(model::CosignatoryModificationType type, const Key& key);

		/// Adds a executor modification around \a type and \a key.
		void addExecutorModification(model::CosignatoryModificationType type, const Key& key);

		/// Adds a verifier modification around \a type and \a key.
		void addVerifierModification(model::CosignatoryModificationType type, const Key& key);

	public:
		/// Builds a new modify multisig account transaction.
		std::unique_ptr<Transaction> build() const;

		/// Builds a new embedded modify multisig account transaction.
		std::unique_ptr<EmbeddedTransaction> buildEmbedded() const;

	private:
		template<typename TTransaction>
		std::unique_ptr<TTransaction> buildImpl() const;

	private:
		int64_t m_durationDelta;
		Key m_multisig;
		Hash256 m_hash;
		std::vector<model::CosignatoryModification> m_customerModifications;
		std::vector<model::CosignatoryModification> m_executorModifications;
		std::vector<model::CosignatoryModification> m_verifierModifications;
	};
}}
