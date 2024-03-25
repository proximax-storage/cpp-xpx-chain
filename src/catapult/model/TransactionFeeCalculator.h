/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once

#include "catapult/model/EntityType.h"
#include "catapult/types.h"

#include <set>

namespace catapult::model { class Transaction; }

namespace catapult::model {

	using TransactionSizeSupplier = std::function<uint32_t (const Transaction&, const Height&)>;

	class TransactionFeeCalculator {
	public:
		void addLimitedFeeTransaction(EntityType entityType, VersionType versionType);
		void addTransactionSizeSupplier(EntityType entityType, TransactionSizeSupplier supplier);

		Amount calculateTransactionFee(
				BlockFeeMultiplier feeMultiplier,
				const Transaction& transaction,
				uint32_t feeInterest,
				uint32_t feeInterestDenominator,
				const Height& height) const;

		bool isTransactionFeeLimited(EntityType entityType, VersionType versionType) const;

	private:
		std::map<EntityType, std::set<VersionType>> m_limitedFeeTransactions;
		std::map<EntityType, TransactionSizeSupplier> m_transactionSizeSuppliers;

	};

}