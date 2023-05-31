/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "AccountV2UpgradeBuilder.h"

namespace catapult { namespace builders {

	AccountV2UpgradeBuilder::AccountV2UpgradeBuilder(model::NetworkIdentifier networkIdentifier, const Key& signer)
		: TransactionBuilder(networkIdentifier, signer)
	{}

	void AccountV2UpgradeBuilder::setNewAccount(const Key& newAccountKey) {
		m_newAccountKey = newAccountKey;
	}

	template<typename TransactionType>
	model::UniqueEntityPtr<TransactionType> AccountV2UpgradeBuilder::buildImpl() const {
		// 1. allocate
		auto pTransaction = createTransaction<TransactionType>(sizeof(TransactionType));

		// 2. set transaction fields
		pTransaction->NewAccountPublicKey = m_newAccountKey;
		return pTransaction;
	}

	model::UniqueEntityPtr<AccountV2UpgradeBuilder::Transaction> AccountV2UpgradeBuilder::build() const {
		return buildImpl<Transaction>();
	}

	model::UniqueEntityPtr<AccountV2UpgradeBuilder::EmbeddedTransaction> AccountV2UpgradeBuilder::buildEmbedded() const {
		return buildImpl<EmbeddedTransaction>();
	}
}}
