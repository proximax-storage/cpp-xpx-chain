/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "AddHarvesterBuilder.h"

namespace catapult { namespace builders {

	AddHarvesterBuilder::AddHarvesterBuilder(model::NetworkIdentifier networkIdentifier, const Key& signer, const Key& harvesterKey)
		: TransactionBuilder(networkIdentifier, signer)
		, m_harvesterKey(harvesterKey)
	{}

	template<typename TransactionType>
	model::UniqueEntityPtr<TransactionType> AddHarvesterBuilder::buildImpl() const {
		auto pTransaction = createTransaction<TransactionType>(sizeof(TransactionType));
		pTransaction->HarvesterKey = m_harvesterKey;
		return pTransaction;
	}

	model::UniqueEntityPtr<AddHarvesterBuilder::Transaction> AddHarvesterBuilder::build() const {
		return buildImpl<Transaction>();
	}

	model::UniqueEntityPtr<AddHarvesterBuilder::EmbeddedTransaction> AddHarvesterBuilder::buildEmbedded() const {
		return buildImpl<EmbeddedTransaction>();
	}
}}
