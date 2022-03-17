/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "LockFundEntityType.h"
#include "catapult/model/Mosaic.h"
#include "catapult/model/Transaction.h"
#include "catapult/model/LockFundAction.h"

namespace catapult { namespace model {

#pragma pack(push, 1)

	/// Binary layout for a lcok fund transaction body.
	template<typename THeader>
	struct LockFundTransferTransactionBody : public THeader {
	private:
		using TransactionType = LockFundTransferTransactionBody<THeader>;

	public:
		DEFINE_TRANSACTION_CONSTANTS(Entity_Type_Lock_Fund_Transfer, 1)

	public:

		/// Number of blocks until unlock. Must be higher than configured minimum.
		BlockDuration Duration;
		/// Number of mosaics.
		uint8_t MosaicsCount;
		/// Number of blocks until unlock. Must be higher than configured minimum.
		model::LockFundAction Action;
		// followed by mosaics data if MosaicsCount != 0
		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(Mosaics, UnresolvedMosaic)

	private:

		template<typename T>
		static auto* MosaicsPtrT(T& transaction) {
			auto* pPayloadStart = THeader::PayloadStart(transaction);
			return transaction.MosaicsCount && pPayloadStart ? pPayloadStart : nullptr;
		}

	public:
		// Calculates the real size of transfer \a transaction.
		static constexpr uint64_t CalculateRealSize(const TransactionType& transaction) noexcept {
			return sizeof(TransactionType) + transaction.MosaicsCount * sizeof(UnresolvedMosaic);
		}
	};

	DEFINE_EMBEDDABLE_TRANSACTION(LockFundTransfer)

#pragma pack(pop)
}}
