/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "OperationEntityType.h"
#include "OperationTypes.h"
#include "catapult/model/Transaction.h"
#include "catapult/utils/ArraySet.h"

namespace catapult { namespace model {

#pragma pack(push, 1)

	/// Binary layout for an end operation transaction body.
	template<typename THeader>
	struct EndOperationTransactionBody : public BasicOperationTransactionBody<THeader> {
	private:
		using TransactionType = EndOperationTransactionBody<THeader>;
		using BaseTransactionType = BasicOperationTransactionBody<THeader>;

	public:
		DEFINE_TRANSACTION_CONSTANTS(Entity_Type_EndOperation, 1)

	public:
		/// Operation token.
		Hash256 OperationToken;

		/// Operation result.
		OperationResult Result;

	public:
		// Calculates the real size of an end operation transaction.
		static constexpr uint64_t CalculateRealSize(const TransactionType& transaction) noexcept {
			return BaseTransactionType::CalculateRealSize(transaction) + sizeof(Hash256);
		}
	};

	DEFINE_EMBEDDABLE_TRANSACTION(EndOperation)

#pragma pack(pop)

	/// Extracts public keys of additional accounts that must approve \a transaction.
	inline utils::KeySet ExtractAdditionalRequiredCosigners(const EmbeddedEndOperationTransaction&) {
		return {};
	}
}}
