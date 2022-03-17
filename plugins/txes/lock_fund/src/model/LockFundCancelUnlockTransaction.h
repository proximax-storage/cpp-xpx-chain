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
	struct LockFundCancelUnlockTransactionBody : public THeader {
	private:
		using TransactionType = LockFundCancelUnlockTransactionBody<THeader>;

	public:
		DEFINE_TRANSACTION_CONSTANTS(Entity_Type_Lock_Fund_Cancel_Unlock, 1)

	public:

		/// Height at which to cancel the unlock request for this signer.
		Height TargetHeight;

	public:
		// Calculates the real size of hash lock \a transaction.
		static constexpr uint64_t CalculateRealSize(const TransactionType&) noexcept {
			return sizeof(TransactionType);
		}
	};

	DEFINE_EMBEDDABLE_TRANSACTION(LockFundCancelUnlock)

#pragma pack(pop)
}}
