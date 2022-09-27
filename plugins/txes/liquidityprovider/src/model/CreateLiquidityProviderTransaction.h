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
	struct CreateLiquidityProviderTransactionBody : public THeader {
	private:
		using TransactionType = CreateLiquidityProviderTransactionBody<THeader>;

	public:
		explicit CreateLiquidityProviderTransactionBody<THeader>() {};

	public:
		DEFINE_TRANSACTION_CONSTANTS(Entity_Type_CreateLiquidityProvider, 1)

	public:
		UnresolvedMosaicId ProviderMosaicId;
		Amount CurrencyDeposit;
		Amount InitialMosaicsMinting;
		uint32_t SlashingPeriod;
		uint16_t WindowSize;
		Key SlashingAccount;
		uint32_t Alpha;
		uint32_t Beta;


	public:
		// Calculates the real size of a storage \a transaction.
		static constexpr uint64_t CalculateRealSize(const TransactionType&) noexcept {
			return sizeof(TransactionType);
		}
	};

	DEFINE_EMBEDDABLE_TRANSACTION(CreateLiquidityProvider)

#pragma pack(pop)
}}
