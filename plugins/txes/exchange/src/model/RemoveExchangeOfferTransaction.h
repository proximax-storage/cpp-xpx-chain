/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "ExchangeEntityType.h"
#include "Offer.h"
#include "catapult/model/Transaction.h"

namespace catapult { namespace model {

#pragma pack(push, 1)

	/// Binary layout for a remove exchange offer transaction body.
	template<typename THeader>
	struct RemoveExchangeOfferTransactionBody : public THeader {
	private:
		using TransactionType = RemoveExchangeOfferTransactionBody<THeader>;

	public:
		DEFINE_TRANSACTION_CONSTANTS(Entity_Type_Remove_Exchange_Offer, 2)

	public:
		uint8_t OfferCount;

		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(Offers, OfferMosaic)

	private:
		template<typename T>
		static auto* OffersPtrT(T& transaction) {
			return transaction.OfferCount ? THeader::PayloadStart(transaction) : nullptr;
		}

	public:
		// Calculates the real size of a remove exchange offer transaction.
		static constexpr uint64_t CalculateRealSize(const TransactionType& transaction) noexcept {
			return sizeof(TransactionType) + transaction.OfferCount * sizeof(OfferMosaic);
		}
	};

	DEFINE_EMBEDDABLE_TRANSACTION(RemoveExchangeOffer)

#pragma pack(pop)
}}
