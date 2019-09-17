/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "ServiceEntityType.h"
#include "catapult/model/Transaction.h"
#include "Action.h"

namespace catapult { namespace model {

#pragma pack(push, 1)

	/// Binary layout for a service transaction body.
	template<typename THeader>
	struct ServiceTransactionBody : public THeader {
	private:
		using TransactionType = ServiceTransactionBody<THeader>;

	public:
		DEFINE_TRANSACTION_CONSTANTS(Entity_Type_Service, 1)

	public:
		/// Mosaic recipient.
		Key Recipient;

		/// Number of mosaics.
		uint8_t MosaicsCount;

		/// Action type.
		DriveActionType ActionType;

		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(Mosaics, UnresolvedMosaic)

		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(Action, uint8_t)

	private:
		template<typename T>
		static auto* MosaicsPtrT(T& transaction) {
			return transaction.MosaicsCount ? THeader::PayloadStart(transaction) : nullptr;
		}

		template<typename T>
		static auto* ActionPtrT(T& transaction) {
			auto* pPayloadStart = THeader::PayloadStart(transaction);
			return pPayloadStart ? pPayloadStart + transaction.MosaicsCount * sizeof(UnresolvedMosaic) : nullptr;
		}

	public:
		// Calculates the real size of a service \a transaction.
		static constexpr uint64_t CalculateRealSize(const TransactionType& transaction) noexcept {
			return sizeof(TransactionType) + transaction.MosaicsCount * sizeof(UnresolvedMosaic)
				+ CalculateActionSize(transaction.ActionType, transaction.ToBytePointer() + sizeof(TransactionType)
					+ transaction.MosaicsCount * sizeof(UnresolvedMosaic));
		}
	};

	DEFINE_EMBEDDABLE_TRANSACTION(Service)

#pragma pack(pop)
}}
