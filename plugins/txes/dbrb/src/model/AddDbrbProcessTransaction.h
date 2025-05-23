/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "DbrbEntityType.h"
#include "catapult/model/Transaction.h"

namespace catapult { namespace model {

#pragma pack(push, 1)

	/// Binary layout for an add DBRB process transaction body.
	template<typename THeader>
	struct AddDbrbProcessTransactionBody : public THeader {
	private:
		using TransactionType = AddDbrbProcessTransactionBody<THeader>;

	public:
		DEFINE_TRANSACTION_CONSTANTS(Entity_Type_AddDbrbProcess, 2)

	public:
		uint16_t HarvesterKeysCount;

		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(HarvesterKeys, Key)

	private:
		template<typename T>
		static auto* HarvesterKeysPtrT(T& transaction) {
			return transaction.HarvesterKeysCount ? THeader::PayloadStart(transaction) : nullptr;
		}

	public:
		// Calculates the real size of \a transaction.
		static constexpr uint64_t CalculateRealSize(const TransactionType& transaction) noexcept {
			return sizeof(TransactionType) + transaction.HarvesterKeysCount * Key_Size;
		}
	};

	DEFINE_EMBEDDABLE_TRANSACTION(AddDbrbProcess)

#pragma pack(pop)
}}
