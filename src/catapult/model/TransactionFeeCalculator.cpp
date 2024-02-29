/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "TransactionFeeCalculator.h"
#include "catapult/model/Transaction.h"

namespace catapult::model {

	void TransactionFeeCalculator::addLimitedFeeTransaction(EntityType entityType, VersionType versionType) {
		auto [entitiesIt, _] = m_limitedFeeTransactions.try_emplace(entityType, std::set<VersionType>());
		entitiesIt->second.insert(versionType);
	}

	void TransactionFeeCalculator::addTransactionSizeSupplier(EntityType entityType, TransactionSizeSupplier supplier) {
		m_transactionSizeSuppliers.emplace(entityType, supplier);
	}

	Amount TransactionFeeCalculator::calculateTransactionFee(
			BlockFeeMultiplier feeMultiplier,
			const Transaction& transaction,
			uint32_t feeInterest,
			uint32_t feeInterestDenominator,
			const Height& height) const {

        auto size = transaction.Size;
		auto iter = m_transactionSizeSuppliers.find(transaction.Type);
        if (iter != m_transactionSizeSuppliers.cend())
            size = iter->second(transaction, height);

		auto fee = Amount(feeMultiplier.unwrap() * size * feeInterest / feeInterestDenominator);
		if (isTransactionFeeLimited(transaction.Type, transaction.EntityVersion()))
			return std::min(transaction.MaxFee, fee);

		return fee;
	}

	bool TransactionFeeCalculator::isTransactionFeeLimited(EntityType entityType, VersionType versionType) const {
		auto entityIt = m_limitedFeeTransactions.find(entityType);
		if (entityIt == m_limitedFeeTransactions.end())
			return false;

		return entityIt->second.find(versionType) != entityIt->second.end();
	}

} // namespace catapult::model