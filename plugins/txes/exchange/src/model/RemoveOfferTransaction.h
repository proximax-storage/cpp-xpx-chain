/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "ExchangeEntityType.h"
#include "catapult/model/Transaction.h"
#include "catapult/utils/ShortHash.h"

namespace catapult { namespace model {

#pragma pack(push, 1)

	/// Binary layout for a base exchange transaction body.
	template<typename THeader>
	struct RemoveOfferTransactionBody : public THeader {
	private:
		using TransactionType = RemoveOfferTransactionBody<THeader>;

	public:
		DEFINE_TRANSACTION_CONSTANTS(Entity_Type_Remove_Offer, 1)

	public:
		uint8_t OfferCount;

		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(OfferHashes, utils::ShortHash)

	private:
		template<typename T>
		static auto* OfferHashesPtrT(T& transaction) {
			return transaction.OfferCount ? THeader::PayloadStart(transaction) : nullptr;
		}

	public:
		// Calculates the real size of an exchange transaction.
		static constexpr uint64_t CalculateRealSize(const TransactionType& transaction) noexcept {
			return sizeof(TransactionType) + transaction.OfferCount * sizeof(utils::ShortHash);
		}
	};

	DEFINE_EMBEDDABLE_TRANSACTION(RemoveOffer)

#pragma pack(pop)
}}
