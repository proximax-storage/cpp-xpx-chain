/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "OperationEntityType.h"
#include "OperationTypes.h"
#include "catapult/model/Transaction.h"

namespace catapult { namespace model {

#pragma pack(push, 1)

	/// Binary layout for a start operation transaction body.
	template<typename THeader>
	struct StartOperationTransactionBody : public BasicStartOperationTransactionBody<THeader, StartOperationTransactionBody<THeader>> {
	private:
		using TransactionType = StartOperationTransactionBody<THeader>;
		using BaseTransactionType = BasicStartOperationTransactionBody<THeader, StartOperationTransactionBody<THeader>>;

	public:
		DEFINE_TRANSACTION_CONSTANTS(Entity_Type_StartOperation, 1)

	public:
		/// Number of executors.
		uint8_t ExecutorCount;

		// Executors.
		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(Executors, Key)

	private:
		template<typename T>
		static auto* ExecutorsPtrT(T& transaction) {
			auto* pPayloadStart = THeader::PayloadStart(transaction);
			return transaction.ExecutorCount && pPayloadStart ? pPayloadStart + transaction.MosaicCount * sizeof(UnresolvedMosaic) : nullptr;
		}

	public:
		// Calculates the real size of a start operation transaction.
		static constexpr uint64_t CalculateRealSize(const TransactionType& transaction) noexcept {
			return BaseTransactionType::CalculateRealSize(transaction) + 1 + transaction.ExecutorCount * sizeof(Key);
		}
	};

	DEFINE_EMBEDDABLE_TRANSACTION(StartOperation)

#pragma pack(pop)
}}
