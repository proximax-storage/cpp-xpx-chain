/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "SuperContractEntityType.h"
#include "catapult/model/Mosaic.h"
#include "catapult/model/SupercontractModel.h"
#include "catapult/model/Transaction.h"
#include "EndBatchExecutionModel.h"
#include <array>

namespace catapult::model {

#pragma pack(push, 1)

	/// Binary layout for a deploy transaction body.
	template<typename THeader>
	struct EndBatchExecutionSingleTransactionBody : public THeader {
	private:
		using TransactionType = EndBatchExecutionSingleTransactionBody<THeader>;

	public:
		DEFINE_TRANSACTION_CONSTANTS(Entity_Type_EndBatchExecutionSingleTransaction, 1)

		/// Contract public key
		Key ContractKey;

		uint64_t BatchId;

		RawProofOfExecution ProofOfExecution;

	public:
		// Calculates the real size of a deploy \a transaction.
		static constexpr uint64_t CalculateRealSize(const TransactionType& transaction) noexcept {
			return sizeof(TransactionType);
		}
	};

	DEFINE_EMBEDDABLE_TRANSACTION(EndBatchExecutionSingle)

#pragma pack(pop)
}