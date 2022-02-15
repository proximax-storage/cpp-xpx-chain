/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
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
