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

	/// Binary layout for an exchange transaction body.
	template<typename THeader>
	struct ExchangeTransactionBody : public THeader {
	private:
		using TransactionType = ExchangeTransactionBody<THeader>;

	public:
		DEFINE_TRANSACTION_CONSTANTS(Entity_Type_Exchange_Offer, 1)

	public:
		uint8_t OfferCount;

		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(Offers, MatchedOffer)

	private:
		template<typename T>
		static auto* OffersPtrT(T& transaction) {
			return transaction.OfferCount ? THeader::PayloadStart(transaction) : nullptr;
		}

	public:
		// Calculates the real size of an exchange transaction.
		static constexpr uint64_t CalculateRealSize(const TransactionType& transaction) noexcept {
			return sizeof(TransactionType) + transaction.OfferCount * sizeof(MatchedOffer);
		}
	};

	DEFINE_EMBEDDABLE_TRANSACTION(Exchange)

#pragma pack(pop)
}}
