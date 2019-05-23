/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "CatapultConfigBuilder.h"

namespace catapult { namespace builders {

	CatapultConfigBuilder::CatapultConfigBuilder(model::NetworkIdentifier networkIdentifier, const Key& signer)
		: TransactionBuilder(networkIdentifier, signer)
	{}

	void CatapultConfigBuilder::setApplyHeightDelta(const BlockDuration& applyHeightDelta) {
		m_applyHeightDelta = applyHeightDelta;
	}

	void CatapultConfigBuilder::setBlockChainConfig(const RawBuffer& blockChainConfig) {
		m_blockChainConfig.resize(blockChainConfig.Size);
		m_blockChainConfig.assign(blockChainConfig.pData, blockChainConfig.pData + blockChainConfig.Size);
	}

	template<typename TransactionType>
	std::unique_ptr<TransactionType> CatapultConfigBuilder::buildImpl() const {
		// 1. allocate, zero (header), set model::Transaction fields
		auto pTransaction = createTransaction<TransactionType>(sizeof(TransactionType) + m_blockChainConfig.size());

		// 2. set fixed transaction fields
		pTransaction->ApplyHeightDelta = m_applyHeightDelta;
		pTransaction->BlockChainConfigSize = utils::checked_cast<size_t, uint32_t>(m_blockChainConfig.size());

		// 3. set transaction attachments
		std::copy(m_blockChainConfig.cbegin(), m_blockChainConfig.cend(), pTransaction->BlockChainConfigPtr());

		return pTransaction;
	}

	std::unique_ptr<CatapultConfigBuilder::Transaction> CatapultConfigBuilder::build() const {
		return buildImpl<Transaction>();
	}

	std::unique_ptr<CatapultConfigBuilder::EmbeddedTransaction> CatapultConfigBuilder::buildEmbedded() const {
		return buildImpl<EmbeddedTransaction>();
	}
}}
