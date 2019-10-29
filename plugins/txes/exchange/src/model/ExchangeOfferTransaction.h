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

	/// Binary layout for an exchange offer transaction body.
	template<typename THeader>
	struct ExchangeOfferTransactionBody : public THeader {
	private:
		using TransactionType = ExchangeOfferTransactionBody<THeader>;

	public:
		DEFINE_TRANSACTION_CONSTANTS(Entity_Type_Exchange_Offer, 1)

	public:
		BlockDuration Duration;

		uint8_t SellOfferCount;

		uint8_t BuyOfferCount;

		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(SellOffers, Offer)

		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(BuyOffers, Offer)

	private:
		template<typename T>
		static auto* SellOffersPtrT(T& transaction) {
			return transaction.SellOfferCount ? THeader::PayloadStart(transaction) : nullptr;
		}

		template<typename T>
		static auto* BuyOffersPtrT(T& transaction) {
			auto* pPayloadStart = THeader::PayloadStart(transaction);
			return transaction.BuyOfferCount && pPayloadStart ? pPayloadStart + transaction.SellOfferCount * sizeof(Offer) : nullptr;
		}

	public:
		// Calculates the real size of an exchange offer transaction.
		static constexpr uint64_t CalculateRealSize(const TransactionType& transaction) noexcept {
			return sizeof(TransactionType) + (transaction.SellOfferCount + transaction.BuyOfferCount) * sizeof(Offer);
		}
	};

	DEFINE_EMBEDDABLE_TRANSACTION(ExchangeOffer)

#pragma pack(pop)
}}
