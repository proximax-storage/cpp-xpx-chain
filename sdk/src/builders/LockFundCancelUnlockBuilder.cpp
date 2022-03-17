/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "LockFundCancelUnlockBuilder.h"

namespace catapult { namespace builders {

	LockFundCancelUnlockBuilder::LockFundCancelUnlockBuilder(model::NetworkIdentifier networkIdentifier, const Key& signer)
			: TransactionBuilder(networkIdentifier, signer)
			, m_targetHeight()
	{}

	void LockFundCancelUnlockBuilder::setHeight(Height height)
	{
		m_targetHeight = height;
	}


	model::UniqueEntityPtr<LockFundCancelUnlockBuilder::Transaction> LockFundCancelUnlockBuilder::build() const {
		return buildImpl<Transaction>();
	}

	model::UniqueEntityPtr<LockFundCancelUnlockBuilder::EmbeddedTransaction> LockFundCancelUnlockBuilder::buildEmbedded() const {
		return buildImpl<EmbeddedTransaction>();
	}

	template<typename TransactionType>
	model::UniqueEntityPtr<TransactionType> LockFundCancelUnlockBuilder::buildImpl() const {
		// 1. allocate, zero (header), set model::Transaction fields
		auto size = sizeof(TransactionType);
		auto pTransaction = createTransaction<TransactionType>(size);

		// 2. set fixed transaction fields
		pTransaction->TargetHeight = m_targetHeight;

		return pTransaction;
	}
}}
