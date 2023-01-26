/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "SynchronizationSingleBuilder.h"

namespace catapult { namespace builders {
	SynchronizationSingleBuilder::SynchronizationSingleBuilder(model::NetworkIdentifier networkIdentifier, const Key& signer)
		: TransactionBuilder(networkIdentifier, signer)
	{}

	void SynchronizationSingleBuilder::setContractKey(const Key& contractKey) {
		m_contractKey = contractKey;
	}

	void SynchronizationSingleBuilder::setBatchId(uint64_t batchId) {
		m_batchId = batchId;
	}

	template<typename TransactionType>
	model::UniqueEntityPtr<TransactionType> SynchronizationSingleBuilder::buildImpl() const {
		// 1. allocate
		auto size = sizeof(TransactionType);
		auto pTransaction = createTransaction<TransactionType>(size);

		// 2. set transaction fields
		pTransaction->ContractKey = m_contractKey;
		pTransaction->BatchId = m_batchId;

		// 3. set transaction attachments

		return pTransaction;
	}

	model::UniqueEntityPtr<SynchronizationSingleBuilder::Transaction>
			SynchronizationSingleBuilder::build() const {
		return buildImpl<Transaction>();
	}

	model::UniqueEntityPtr<SynchronizationSingleBuilder::EmbeddedTransaction>
			SynchronizationSingleBuilder::buildEmbedded() const {
		return buildImpl<EmbeddedTransaction>();
	}
}}


