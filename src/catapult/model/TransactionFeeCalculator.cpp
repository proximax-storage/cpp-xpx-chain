/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "TransactionFeeCalculator.h"
#include "catapult/model/Transaction.h"

namespace catapult::model {

	void TransactionFeeCalculator::addUnlimitedFeeTransaction(EntityType entityType, VersionType versionType) {
		auto [entitiesIt, inserted] = m_unlimitedFeeTransactions.try_emplace(entityType, std::set<VersionType>());
		entitiesIt->second.insert(versionType);
	}

	Amount TransactionFeeCalculator::calculateTransactionFee(
			BlockFeeMultiplier feeMultiplier,
			const Transaction& transaction,
			uint32_t feeInterest,
			uint32_t feeInterestDenominator) const {
		auto fee = Amount(feeMultiplier.unwrap() * transaction.Size * feeInterest / feeInterestDenominator);
		if (isTransactionFeeUnlimited(transaction.Type, transaction.EntityVersion())) {
			return std::min(transaction.MaxFee, fee);
		}
		return fee;
	}

	bool TransactionFeeCalculator::isTransactionFeeUnlimited(EntityType entityType,
															 VersionType versionType) const {
		auto entityIt = m_unlimitedFeeTransactions.find(entityType);
		if (entityIt == m_unlimitedFeeTransactions.end()) {
			return false;
		}
		return entityIt->second.find(versionType) != entityIt->second.end();
	}

} // namespace catapult::model