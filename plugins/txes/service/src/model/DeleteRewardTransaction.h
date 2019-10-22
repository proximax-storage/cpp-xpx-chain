/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "ServiceEntityType.h"
#include "ServiceTypes.h"
#include "catapult/model/Transaction.h"

namespace catapult { namespace model {

#pragma pack(push, 1)

	/// Binary layout for a delete reward transaction body.
	template<typename THeader>
	struct DeleteRewardTransactionBody : public THeader {
	private:
		using TransactionType = DeleteRewardTransactionBody<THeader>;

	public:
		DEFINE_TRANSACTION_CONSTANTS(Entity_Type_DeleteReward, 1)

		/// TODO: Transaction is not implemented

	public:
		// Calculates the real size of a service \a transaction.
		static constexpr uint64_t CalculateRealSize(const TransactionType& transaction) noexcept {
			return sizeof(TransactionType) + transaction.FilesCount * sizeof(File);
		}
	};

	DEFINE_EMBEDDABLE_TRANSACTION(DeleteReward)

#pragma pack(pop)
}}
