/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "SdaExchangeEntityType.h"
#include "SdaOffer.h"
#include "catapult/model/Transaction.h"

namespace catapult { namespace model {

#pragma pack(push, 1)

	/// Binary layout for an SDA-SDA exchange transaction body.
	template<typename THeader>
	struct ExchangeSdaTransactionBody : public THeader {
	private:
		using TransactionType = ExchangeSdaTransactionBody<THeader>;

	public:
		DEFINE_TRANSACTION_CONSTANTS(Entity_Type_Sda_Exchange, 1)

	public:
		uint8_t SdaOfferCount;

		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(SdaOffers, MatchedSdaOffer)

	private:
		template<typename T>
		static auto* SdaOffersPtrT(T& transaction) {
			return transaction.OfferCount ? THeader::PayloadStart(transaction) : nullptr;
		}

	public:
		// Calculates the real size of an exchange transaction.
		static constexpr uint64_t CalculateRealSize(const TransactionType& transaction) noexcept {
			return sizeof(TransactionType) + transaction.SdaOfferCount * sizeof(MatchedSdaOffer);
		}
	};

	DEFINE_EMBEDDABLE_TRANSACTION(ExchangeSda)

#pragma pack(pop)
}}
