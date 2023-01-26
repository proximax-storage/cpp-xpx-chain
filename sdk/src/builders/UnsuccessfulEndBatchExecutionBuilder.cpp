/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "UnsuccessfulEndBatchExecutionBuilder.h"

namespace catapult { namespace builders {
	UnsuccessfulEndBatchExecutionBuilder::UnsuccessfulEndBatchExecutionBuilder(model::NetworkIdentifier networkIdentifier, const Key& signer)
		: TransactionBuilder(networkIdentifier, signer)
		, m_cosignersNumber(0)
		, m_callsNumber(0)
	{}

	void UnsuccessfulEndBatchExecutionBuilder::setContractKey(const Key& contractKey) {
		m_contractKey = contractKey;
	}

	void UnsuccessfulEndBatchExecutionBuilder::setBatchId(uint64_t batchId) {
		m_batchId = batchId;
	}

	void UnsuccessfulEndBatchExecutionBuilder::setAutomaticExecutionsNextBlockToCheck(Height automaticExecutionsNextBlockToCheck) {
		m_automaticExecutionsNextBlockToCheck = automaticExecutionsNextBlockToCheck;
	}

	void UnsuccessfulEndBatchExecutionBuilder::setCosignersNumber(uint16_t cosignersNumber) {
		m_cosignersNumber = cosignersNumber;
	}

	void UnsuccessfulEndBatchExecutionBuilder::setCallsNumber(uint16_t callsNumber) {
		m_callsNumber = callsNumber;
	}

	void UnsuccessfulEndBatchExecutionBuilder::setPublicKeys(std::vector<Key>&& publicKeys) {
		m_publicKeys = std::move(publicKeys);
	}

	void UnsuccessfulEndBatchExecutionBuilder::setSignatures(std::vector<Signature>&& signatures) {
		m_signatures = std::move(signatures);
	}

	void UnsuccessfulEndBatchExecutionBuilder::setProofsOfExecution(std::vector<model::RawProofOfExecution>&& proofsOfExecution) {
		m_proofsOfExecution = std::move(proofsOfExecution);
	}

	void UnsuccessfulEndBatchExecutionBuilder::setCallDigests(std::vector<model::ShortCallDigest>&& callDigests) {
		m_callDigests = std::move(callDigests);
	}

	void UnsuccessfulEndBatchExecutionBuilder::setCallPayments(std::vector<model::CallPayment>&& callPayments) {
		m_callPayments = std::move(callPayments);
	}

	template<typename TransactionType>
	model::UniqueEntityPtr<TransactionType> UnsuccessfulEndBatchExecutionBuilder::buildImpl() const {
		// 1. allocate
		auto size = sizeof(TransactionType)
					+ m_publicKeys.size() * Key_Size
					+ m_signatures.size() * Signature_Size
					+ m_proofsOfExecution.size() * sizeof(model::RawProofOfExecution)
					+ m_callDigests.size() * sizeof(model::ShortCallDigest)
					+ m_callPayments.size() * sizeof(model::CallPayment);
		auto pTransaction = createTransaction<TransactionType>(size);

		// 2. set transaction fields
		pTransaction->ContractKey = m_contractKey;
		pTransaction->BatchId = m_batchId;
		pTransaction->AutomaticExecutionsNextBlockToCheck = m_automaticExecutionsNextBlockToCheck;
		pTransaction->CosignersNumber = m_cosignersNumber;
		pTransaction->CallsNumber = m_callsNumber;

		// 3. set transaction attachments
		std::copy(m_publicKeys.cbegin(), m_publicKeys.cend(), pTransaction->PublicKeysPtr());
		std::copy(m_signatures.cbegin(), m_signatures.cend(), pTransaction->SignaturesPtr());
		std::copy(m_proofsOfExecution.cbegin(), m_proofsOfExecution.cend(), pTransaction->ProofsOfExecutionPtr());
		std::copy(m_callDigests.cbegin(), m_callDigests.cend(), pTransaction->CallDigestsPtr());
		std::copy(m_callPayments.cbegin(), m_callPayments.cend(), pTransaction->CallPaymentsPtr());
		return pTransaction;
	}

	model::UniqueEntityPtr<UnsuccessfulEndBatchExecutionBuilder::Transaction>
			UnsuccessfulEndBatchExecutionBuilder::build() const {
		return buildImpl<Transaction>();
	}

	model::UniqueEntityPtr<UnsuccessfulEndBatchExecutionBuilder::EmbeddedTransaction>
			UnsuccessfulEndBatchExecutionBuilder::buildEmbedded() const {
		return buildImpl<EmbeddedTransaction>();
	}
}}


