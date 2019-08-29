/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "BlockchainUpgradeBuilder.h"

namespace catapult { namespace builders {

	BlockchainUpgradeBuilder::BlockchainUpgradeBuilder(model::NetworkIdentifier networkIdentifier, const Key& signer)
		: TransactionBuilder(networkIdentifier, signer)
	{}

	void BlockchainUpgradeBuilder::setUpgradePeriod(const BlockDuration& upgradePeriod) {
		m_upgradePeriod = upgradePeriod;
	}

	void BlockchainUpgradeBuilder::setNewBlockchainVersion(const BlockchainVersion& newBlockchainVersion) {
		m_newBlockchainVersion = newBlockchainVersion;
	}

	template<typename TransactionType>
	std::unique_ptr<TransactionType> BlockchainUpgradeBuilder::buildImpl() const {
		// 1. allocate
		auto pTransaction = createTransaction<TransactionType>(sizeof(TransactionType));

		// 2. set transaction fields
		pTransaction->UpgradePeriod = m_upgradePeriod;
		pTransaction->NewBlockchainVersion = m_newBlockchainVersion;

		return pTransaction;
	}

	std::unique_ptr<BlockchainUpgradeBuilder::Transaction> BlockchainUpgradeBuilder::build() const {
		return buildImpl<Transaction>();
	}

	std::unique_ptr<BlockchainUpgradeBuilder::EmbeddedTransaction> BlockchainUpgradeBuilder::buildEmbedded() const {
		return buildImpl<EmbeddedTransaction>();
	}
}}
