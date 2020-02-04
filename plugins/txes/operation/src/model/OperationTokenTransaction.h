/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "OperationEntityType.h"
#include "catapult/model/Transaction.h"

namespace catapult { namespace model {

#pragma pack(push, 1)

	/// Binary layout for an operation token transaction body.
	template<typename THeader>
	struct OperationTokenTransactionBody : public THeader {
	private:
		using TransactionType = OperationTokenTransactionBody<THeader>;

	public:
		DEFINE_TRANSACTION_CONSTANTS(Entity_Type_OperationToken, 1)

	public:
		/// Operation token.
		Hash256 OperationToken;

	public:
		// Calculates the real size of an operation token transaction.
		static constexpr uint64_t CalculateRealSize(const TransactionType&) noexcept {
			return sizeof(TransactionType);
		}
	};

	DEFINE_EMBEDDABLE_TRANSACTION(OperationToken)

#pragma pack(pop)
}}
