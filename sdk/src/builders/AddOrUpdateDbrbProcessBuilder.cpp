/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "AddOrUpdateDbrbProcessBuilder.h"

namespace catapult { namespace builders {

	AddOrUpdateDbrbProcessBuilder::AddOrUpdateDbrbProcessBuilder(model::NetworkIdentifier networkIdentifier, const Key& signer)
		: TransactionBuilder(networkIdentifier, signer)
	{}

	void AddOrUpdateDbrbProcessBuilder::addHarvesterKey(const Key& harvesterKey) {
		m_harvesterKeys.emplace_back(harvesterKey);
	}

	void AddOrUpdateDbrbProcessBuilder::setBlockchainVersion(const BlockchainVersion& blockchainVersion) {
		m_blockchainVersion = blockchainVersion;
	}

	template<typename TransactionType>
	model::UniqueEntityPtr<TransactionType> AddOrUpdateDbrbProcessBuilder::buildImpl() const {
		auto size = sizeof(TransactionType) + m_harvesterKeys.size() * Key_Size;
		auto pTransaction = createTransaction<TransactionType>(size);
		pTransaction->BlockchainVersion = m_blockchainVersion;
		pTransaction->HarvesterKeysCount = utils::checked_cast<size_t, uint16_t>(m_harvesterKeys.size());
		std::copy(m_harvesterKeys.cbegin(), m_harvesterKeys.cend(), pTransaction->HarvesterKeysPtr());
		return pTransaction;
	}

	model::UniqueEntityPtr<AddOrUpdateDbrbProcessBuilder::Transaction> AddOrUpdateDbrbProcessBuilder::build() const {
		return buildImpl<Transaction>();
	}

	model::UniqueEntityPtr<AddOrUpdateDbrbProcessBuilder::EmbeddedTransaction> AddOrUpdateDbrbProcessBuilder::buildEmbedded() const {
		return buildImpl<EmbeddedTransaction>();
	}
}}
