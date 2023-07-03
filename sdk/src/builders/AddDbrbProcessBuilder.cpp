/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "AddDbrbProcessBuilder.h"

namespace catapult { namespace builders {

	AddDbrbProcessBuilder::AddDbrbProcessBuilder(model::NetworkIdentifier networkIdentifier, const Key& signer)
		: TransactionBuilder(networkIdentifier, signer)
	{}

	void AddDbrbProcessBuilder::addHarvesterKey(const Key& harvesterKey) {
		m_harvesterKeys.emplace_back(harvesterKey);
	}

	template<typename TransactionType>
	model::UniqueEntityPtr<TransactionType> AddDbrbProcessBuilder::buildImpl() const {
		auto size = sizeof(TransactionType) + m_harvesterKeys.size() * Key_Size;
		auto pTransaction = createTransaction<TransactionType>(size);
		pTransaction->HarvesterKeysCount = utils::checked_cast<size_t, uint16_t>(m_harvesterKeys.size());
		std::copy(m_harvesterKeys.cbegin(), m_harvesterKeys.cend(), pTransaction->HarvesterKeysPtr());
		return pTransaction;
	}

	model::UniqueEntityPtr<AddDbrbProcessBuilder::Transaction> AddDbrbProcessBuilder::build() const {
		return buildImpl<Transaction>();
	}

	model::UniqueEntityPtr<AddDbrbProcessBuilder::EmbeddedTransaction> AddDbrbProcessBuilder::buildEmbedded() const {
		return buildImpl<EmbeddedTransaction>();
	}
}}
