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
		uint8_t BuyOfferCount;

		uint8_t SellOfferCount;

		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(BuyOfferHashes, utils::ShortHash)

		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(SellOfferHashes, utils::ShortHash)

	private:
		template<typename T>
		static auto* BuyOfferHashesPtrT(T& transaction) {
			return transaction.BuyOfferCount ? THeader::PayloadStart(transaction) : nullptr;
		}

		template<typename T>
		static auto* SellOfferHashesPtrT(T& transaction) {
			auto* pPayloadStart = THeader::PayloadStart(transaction);
			return transaction.SellOfferCount && pPayloadStart ? pPayloadStart + transaction.BuyOfferCount * sizeof(utils::ShortHash) : nullptr;
		}

	public:
		// Calculates the real size of an exchange transaction.
		static constexpr uint64_t CalculateRealSize(const TransactionType& transaction) noexcept {
			return sizeof(TransactionType) + (transaction.BuyOfferCount + transaction.SellOfferCount) * sizeof(utils::ShortHash);
		}
	};

	DEFINE_EMBEDDABLE_TRANSACTION(RemoveOffer)

#pragma pack(pop)
}}
