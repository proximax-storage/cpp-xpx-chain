/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once

#include "TransactionBuilder.h"
#include "plugins/txes/supercontract_v2/src/model/EndBatchExecutionSingleTransaction.h"

namespace catapult { namespace builders {

	/// Builder for a catapult upgrade transaction.
	class EndBatchExecutionSingleBuilder : public TransactionBuilder {
	public:
		using Transaction = model::EndBatchExecutionSingleTransaction;
		using EmbeddedTransaction = model::EmbeddedEndBatchExecutionSingleTransaction;

		/// Creates a catapult upgrade builder for building a catapult upgrade transaction from \a signer
		/// for the network specified by \a networkIdentifier.
		EndBatchExecutionSingleBuilder(model::NetworkIdentifier networkIdentifier, const Key& signer);

	public:
		void setContractKey(const Key& contractKey);

		void setBatchId(uint64_t batchId);

		void setProofOfExecution(model::RawProofOfExecution proofOfExecution);

	public:
		/// Builds a new data modification approval transaction.
		model::UniqueEntityPtr<Transaction> build() const;

		/// Builds a new embedded data modification approval transaction.
		model::UniqueEntityPtr<EmbeddedTransaction> buildEmbedded() const;

	private:
		template<typename TTransaction>
		model::UniqueEntityPtr<TTransaction> buildImpl() const;

	private:
		Key m_contractKey;
		uint64_t m_batchId;
		model::RawProofOfExecution m_proofsOfExecution;
	};
}}
