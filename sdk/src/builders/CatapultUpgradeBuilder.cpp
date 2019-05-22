/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "CatapultUpgradeBuilder.h"

namespace catapult { namespace builders {

	CatapultUpgradeBuilder::CatapultUpgradeBuilder(model::NetworkIdentifier networkIdentifier, const Key& signer)
		: TransactionBuilder(networkIdentifier, signer)
	{}

	void CatapultUpgradeBuilder::setUpgradePeriod(const uint16_t& upgradePeriod) {
		m_upgradePeriod = upgradePeriod;
	}

	void CatapultUpgradeBuilder::setNewCatapultVersion(const uint64_t& newCatapultVersion) {
		m_newCatapultVersion = newCatapultVersion;
	}

	template<typename TransactionType>
	std::unique_ptr<TransactionType> CatapultUpgradeBuilder::buildImpl() const {
		// 1. allocate
		auto pTransaction = createTransaction<TransactionType>(sizeof(TransactionType));

		// 2. set transaction fields
		pTransaction->UpgradePeriod = m_upgradePeriod;
		pTransaction->NewCatapultVersion = m_newCatapultVersion;

		return pTransaction;
	}

	std::unique_ptr<CatapultUpgradeBuilder::Transaction> CatapultUpgradeBuilder::build() const {
		return buildImpl<Transaction>();
	}

	std::unique_ptr<CatapultUpgradeBuilder::EmbeddedTransaction> CatapultUpgradeBuilder::buildEmbedded() const {
		return buildImpl<EmbeddedTransaction>();
	}
}}
