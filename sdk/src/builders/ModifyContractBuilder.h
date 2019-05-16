/**
*** Copyright 2018 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
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
		Hash256 m_hash;
		std::vector<model::CosignatoryModification> m_customerModifications;
		std::vector<model::CosignatoryModification> m_executorModifications;
		std::vector<model::CosignatoryModification> m_verifierModifications;
	};
}}
