/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "BlockchainUpgradeEntityType.h"
#include "catapult/model/Transaction.h"

namespace catapult { namespace model {

#pragma pack(push, 1)

	/// Binary layout for a catapult upgrade transaction body.
	template<typename THeader>
	struct BlockchainUpgradeTransactionBody : public THeader {
	private:
		using TransactionType = BlockchainUpgradeTransactionBody<THeader>;

	public:
		DEFINE_TRANSACTION_CONSTANTS(Entity_Type_Blockchain_Upgrade, 1)

	public:
		/// Number of blocks before forcing upgrade.
		BlockDuration UpgradePeriod;

		/// New version of catapult.
		BlockchainVersion NewBlockchainVersion;

	public:
		// Calculates the real size of a catapult upgrade transaction.
		static constexpr uint64_t CalculateRealSize(const TransactionType&) noexcept {
			return sizeof(TransactionType);
		}
	};

	DEFINE_EMBEDDABLE_TRANSACTION(BlockchainUpgrade)

#pragma pack(pop)
}}
