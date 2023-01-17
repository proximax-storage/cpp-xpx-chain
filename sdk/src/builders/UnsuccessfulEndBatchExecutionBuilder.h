/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once

#include "TransactionBuilder.h"
#include "plugins/txes/supercontract_v2/src/model/UnsuccessfulEndBatchExecutionTransaction.h"

namespace catapult { namespace builders {

	/// Builder for a catapult upgrade transaction.
	class UnsuccessfulEndBatchExecutionBuilder : public TransactionBuilder {
	public:
		using Transaction = model::UnsuccessfulEndBatchExecutionTransaction;
		using EmbeddedTransaction = model::EmbeddedUnsuccessfulEndBatchExecutionTransaction;

		/// Creates a catapult upgrade builder for building a catapult upgrade transaction from \a signer
		/// for the network specified by \a networkIdentifier.
		UnsuccessfulEndBatchExecutionBuilder(model::NetworkIdentifier networkIdentifier, const Key& signer);

	public:
		void setContractKey(const Key& contractKey);

		void setBatchId(uint64_t batchId);

		void setAutomaticExecutionsNextBlockToCheck(Height automaticExecutionsNextBlockToCheck);

		void setCosignersNumber(uint16_t cosignersNumber);

		void setCallsNumber(uint16_t callsNumber);

		void setPublicKeys(std::vector<Key>&& publicKeys);

		void setSignatures(std::vector<Signature>&& signatures);

		void setProofsOfExecution(std::vector<model::RawProofOfExecution>&& proofsOfExecution);

		void setCallDigests(std::vector<model::ShortCallDigest>&& callDigests);

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
		Height m_automaticExecutionsNextBlockToCheck;
		uint16_t m_cosignersNumber;
		uint16_t m_callsNumber;
		std::vector<Key> m_publicKeys;
		std::vector<Signature> m_signatures;
		std::vector<model::RawProofOfExecution> m_proofsOfExecution;
		std::vector<model::ShortCallDigest> m_callDigests;
		std::vector<model::CallPayment> m_callPayments;
	};
}}
