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

	/// Binary layout for a base exchange offer transaction body.
	template<typename THeader>
	struct BaseOfferTransactionBody : public THeader {
	private:
		using TransactionType = BaseOfferTransactionBody<THeader>;

	public:
		BlockDuration Duration;

		uint8_t OfferCount;

		uint8_t MatchedOfferCount;

		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(Offers, Offer)

		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(MatchedOffers, MatchedOffer)

	private:
		template<typename T>
		static auto* OffersPtrT(T& transaction) {
			return transaction.OfferCount ? THeader::PayloadStart(transaction) : nullptr;
		}

		template<typename T>
		static auto* MatchedOffersPtrT(T& transaction) {
			auto* pPayloadStart = THeader::PayloadStart(transaction);
			return transaction.MatchedOfferCount && pPayloadStart ? pPayloadStart + transaction.OfferCount * sizeof(Offer) : nullptr;
		}

	public:
		// Calculates the real size of an exchange transaction.
		static constexpr uint64_t CalculateRealSize(const TransactionType& transaction) noexcept {
			return sizeof(TransactionType) + transaction.OfferCount * sizeof(Offer) + transaction.MatchedOfferCount * sizeof(MatchedOffer);
		}
	};

#define DEFINE_EXCHANGE_TRANSACTION(OFFER_TYPE) \
	template<typename THeader> \
	struct OFFER_TYPE##OfferTransactionBody : public BaseOfferTransactionBody<THeader> { \
		DEFINE_TRANSACTION_CONSTANTS(Entity_Type_##OFFER_TYPE##_Offer, 1) \
	}; \
	\
	DEFINE_EMBEDDABLE_TRANSACTION(OFFER_TYPE##Offer)

#pragma pack(pop)
}}
