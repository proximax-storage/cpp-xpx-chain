/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "SuccessfulEndBatchExecutionBuilder.h"

namespace catapult { namespace builders {
	SuccessfulEndBatchExecutionBuilder::SuccessfulEndBatchExecutionBuilder(model::NetworkIdentifier networkIdentifier, const Key& signer)
	: TransactionBuilder(networkIdentifier, signer)
	, m_usedSizeBytes(0)
	, m_metaFilesSizeBytes(0)
	, m_cosignersNumber(0)
	, m_callsNumber(0)
	{}

	void SuccessfulEndBatchExecutionBuilder::setContractKey(const Key& contractKey) {
		m_contractKey = contractKey;
	}

	void SuccessfulEndBatchExecutionBuilder::setBatchId(uint64_t batchId) {
		m_batchId = batchId;
	}

	void SuccessfulEndBatchExecutionBuilder::setStorageHash(const Hash256 storageHash) {
		m_storageHash = storageHash;
	}

	void SuccessfulEndBatchExecutionBuilder::setUsedSizeBytes(uint64_t usedSizeBytes) {
		m_usedSizeBytes = usedSizeBytes;
	}

	void SuccessfulEndBatchExecutionBuilder::setMetaFilesSizeBytes(uint64_t metaFilesSizeBytes) {
		m_metaFilesSizeBytes = metaFilesSizeBytes;
	}

	void SuccessfulEndBatchExecutionBuilder::setProofOfExecutionVerificationInformation(std::array<uint8_t, 32>&& proofOfExecutionVerificationInformation) {
		m_proofOfExecutionVerificationInformation = proofOfExecutionVerificationInformation;
	}

	void SuccessfulEndBatchExecutionBuilder::setAutomaticExecutionsNextBlockToCheck(Height automaticExecutionsNextBlockToCheck) {
		m_automaticExecutionsNextBlockToCheck = automaticExecutionsNextBlockToCheck;
	}

	void SuccessfulEndBatchExecutionBuilder::setCosignersNumber(uint16_t cosignersNumber) {
		m_cosignersNumber = cosignersNumber;
	}

	void SuccessfulEndBatchExecutionBuilder::setCallsNumber(uint16_t callsNumber) {
		m_callsNumber = callsNumber;
	}

	void SuccessfulEndBatchExecutionBuilder::setPublicKeys(std::vector<Key>&& publicKeys) {
		m_publicKeys = std::move(publicKeys);
	}

	void SuccessfulEndBatchExecutionBuilder::setSignatures(std::vector<Signature>&& signatures) {
		m_signatures = std::move(signatures);
	}

	void SuccessfulEndBatchExecutionBuilder::setProofsOfExecution(std::vector<model::RawProofOfExecution>&& proofsOfExecution) {
		m_signatures = std::move(proofsOfExecution);
	}

	void SuccessfulEndBatchExecutionBuilder::setCallDigests(std::vector<model::ExtendedCallDigest>&& callDigests) {
		m_signatures = std::move(callDigests);
	}

	void SuccessfulEndBatchExecutionBuilder::setCallPayments(std::vector<model::CallPayment>&& callPayments) {
		m_signatures = std::move(callPayments);
	}

	template<typename TransactionType>
	model::UniqueEntityPtr<TransactionType> SuccessfulEndBatchExecutionBuilder::buildImpl() const {
		// 1. allocate
		// todo confirm poex size
		auto size = sizeof(TransactionType)
		        + m_publicKeys.size() * Key_Size
				+ m_signatures.size() * Signature_Size
				+ m_proofsOfExecution.size() * sizeof(model::RawProofOfExecution)
				+ m_callDigests.size() * sizeof(model::ExtendedCallDigest)
				+ m_callPayments.size() * sizeof(model::CallPayment);
		auto pTransaction = createTransaction<TransactionType>(size);

		// 2. set transaction fields
		pTransaction->ContractKey = m_contractKey;
		pTransaction->BatchId = m_batchId;
		pTransaction->StorageHash = m_storageHash;
		pTransaction->UsedSizeBytes = m_usedSizeBytes;
		pTransaction->MetaFilesSizeBytes = m_metaFilesSizeBytes;
		pTransaction->ProofOfExecutionVerificationInformation = m_proofOfExecutionVerificationInformation;
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

	model::UniqueEntityPtr<SuccessfulEndBatchExecutionBuilder::Transaction>
			SuccessfulEndBatchExecutionBuilder::build() const {
		return buildImpl<Transaction>();
	}

	model::UniqueEntityPtr<SuccessfulEndBatchExecutionBuilder::EmbeddedTransaction>
			SuccessfulEndBatchExecutionBuilder::buildEmbedded() const {
		return buildImpl<EmbeddedTransaction>();
	}
}}


