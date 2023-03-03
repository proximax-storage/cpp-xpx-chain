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

	class TransactionFeeCalculator {

	public:

		void addUnlimitedFeeTransaction(EntityType entityType, VersionType versionType);

		Amount calculateTransactionFee(
				BlockFeeMultiplier feeMultiplier,
				const Transaction& transaction,
				uint32_t feeInterest,
				uint32_t feeInterestDenominator) const;

	private:

		bool isTransactionFeeUnlimited(EntityType entityType, VersionType versionType) const;

	private:

		std::map<EntityType, std::set<VersionType>> m_unlimitedFeeTransactions;

	};

}