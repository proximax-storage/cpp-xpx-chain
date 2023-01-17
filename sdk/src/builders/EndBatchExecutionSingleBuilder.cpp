/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "EndBatchExecutionSingleBuilder.h"

namespace catapult { namespace builders {
	EndBatchExecutionSingleBuilder::EndBatchExecutionSingleBuilder(model::NetworkIdentifier networkIdentifier, const Key& signer)
		: TransactionBuilder(networkIdentifier, signer)
	{}

	void EndBatchExecutionSingleBuilder::setContractKey(const Key& contractKey) {
		m_contractKey = contractKey;
	}

	void EndBatchExecutionSingleBuilder::setBatchId(uint64_t batchId) {
		m_batchId = batchId;
	}

	void EndBatchExecutionSingleBuilder::setProofOfExecution(model::RawProofOfExecution proofOfExecution) {
		m_proofsOfExecution = proofOfExecution;
	}

	template<typename TransactionType>
	model::UniqueEntityPtr<TransactionType> EndBatchExecutionSingleBuilder::buildImpl() const {
		// 1. allocate
		auto size = sizeof(TransactionType);
		auto pTransaction = createTransaction<TransactionType>(size);

		// 2. set transaction fields
		pTransaction->ContractKey = m_contractKey;
		pTransaction->BatchId = m_batchId;
		pTransaction->ProofOfExecution = m_proofsOfExecution;

		// 3. set transaction attachments

		return pTransaction;
	}

	model::UniqueEntityPtr<EndBatchExecutionSingleBuilder::Transaction>
			EndBatchExecutionSingleBuilder::build() const {
		return buildImpl<Transaction>();
	}

	model::UniqueEntityPtr<EndBatchExecutionSingleBuilder::EmbeddedTransaction>
			EndBatchExecutionSingleBuilder::buildEmbedded() const {
		return buildImpl<EmbeddedTransaction>();
	}
}}


