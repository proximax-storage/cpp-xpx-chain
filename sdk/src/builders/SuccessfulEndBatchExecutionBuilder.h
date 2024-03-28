/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once

#include "TransactionBuilder.h"
#include "plugins/txes/supercontract_v2/src/model/SuccessfulEndBatchExecutionTransaction.h"

namespace catapult { namespace builders {

	/// Builder for a catapult upgrade transaction.
	class SuccessfulEndBatchExecutionBuilder : public TransactionBuilder {
	public:
		using Transaction = model::SuccessfulEndBatchExecutionTransaction;
		using EmbeddedTransaction = model::EmbeddedSuccessfulEndBatchExecutionTransaction;

		/// Creates a catapult upgrade builder for building a catapult upgrade transaction from \a signer
		/// for the network specified by \a networkIdentifier.
		SuccessfulEndBatchExecutionBuilder(model::NetworkIdentifier networkIdentifier, const Key& signer);

	public:
		void setContractKey(const Key& contractKey);

		void setBatchId(uint64_t batchId);

		void setStorageHash(const Hash256 storageHash);

		void setUsedSizeBytes(uint64_t usedSizeBytes);

		void setMetaFilesSizeBytes(uint64_t metaFilesSizeBytes);

		void setProofOfExecutionVerificationInformation(std::array<uint8_t, 32>&& proofOfExecutionVerificationInformation);

		void setAutomaticExecutionsNextBlockToCheck(Height automaticExecutionsNextBlockToCheck);

		void setCosignersNumber(uint16_t cosignersNumber);

		void setCallsNumber(uint16_t callsNumber);

		void setPublicKeys(std::vector<Key>&& publicKeys);

		void setSignatures(std::vector<Signature>&& signatures);

		void setProofsOfExecution(std::vector<model::RawProofOfExecution>&& proofsOfExecution);

		void setCallDigests(std::vector<model::ExtendedCallDigest>&& callDigests);

		void setCallPayments(std::vector<model::CallPayment>&& callPayments);

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
		Hash256 m_storageHash;
		uint64_t m_usedSizeBytes;
		uint64_t m_metaFilesSizeBytes;
		std::array<uint8_t, 32> m_proofOfExecutionVerificationInformation;
		Height m_automaticExecutionsNextBlockToCheck;
		uint16_t m_cosignersNumber;
		uint16_t m_callsNumber;
		std::vector<Key> m_publicKeys;
		std::vector<Signature> m_signatures;
		std::vector<model::RawProofOfExecution> m_proofsOfExecution;
		std::vector<model::ExtendedCallDigest> m_callDigests;
		std::vector<model::CallPayment> m_callPayments;
	};
}}
