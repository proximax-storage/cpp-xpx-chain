/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "AddHarvesterBuilder.h"

namespace catapult { namespace builders {

	AddHarvesterBuilder::AddHarvesterBuilder(model::NetworkIdentifier networkIdentifier, const Key& signer)
		: TransactionBuilder(networkIdentifier, signer)
	{}

	template<typename TransactionType>
	model::UniqueEntityPtr<TransactionType> AddHarvesterBuilder::buildImpl() const {
		return createTransaction<TransactionType>(sizeof(TransactionType));
	}

	model::UniqueEntityPtr<AddHarvesterBuilder::Transaction> AddHarvesterBuilder::build() const {
		return buildImpl<Transaction>();
	}

	model::UniqueEntityPtr<AddHarvesterBuilder::EmbeddedTransaction> AddHarvesterBuilder::buildEmbedded() const {
		return buildImpl<EmbeddedTransaction>();
	}
}}
