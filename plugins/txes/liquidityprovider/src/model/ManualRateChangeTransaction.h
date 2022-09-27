/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "LiquidityProviderEntityType.h"
#include "catapult/model/Transaction.h"

namespace catapult { namespace model {

#pragma pack(push, 1)

	template<typename THeader>
	struct ManualRateChangeTransactionBody : public THeader {
	private:
		using TransactionType = ManualRateChangeTransactionBody<THeader>;

	public:
		explicit ManualRateChangeTransactionBody<THeader>() {};

	public:
		DEFINE_TRANSACTION_CONSTANTS(Entity_Type_ManualRateChange, 1)

	public:
		UnresolvedMosaicId ProviderMosaicId;
		bool CurrencyBalanceIncrease;
		Amount CurrencyBalanceChange;
		bool MosaicBalanceIncrease;
		Amount MosaicBalanceChange;

	public:
		// Calculates the real size of a storage \a transaction.
		static constexpr uint64_t CalculateRealSize(const TransactionType&) noexcept {
			return sizeof(TransactionType);
		}
	};

	DEFINE_EMBEDDABLE_TRANSACTION(ManualRateChange)

#pragma pack(pop)
}}
