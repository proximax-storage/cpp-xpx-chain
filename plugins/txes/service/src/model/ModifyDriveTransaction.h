/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "ServiceEntityType.h"
#include "catapult/model/Transaction.h"

namespace catapult { namespace model {

#pragma pack(push, 1)

	/// Binary layout for a modify drive transaction body.
	template<typename THeader>
	struct ModifyDriveTransactionBody : public THeader {
	private:
		using TransactionType = ModifyDriveTransactionBody<THeader>;

	public:
		DEFINE_TRANSACTION_CONSTANTS(Entity_Type_ModifyDrive, 1)

	public:
		/// Price delta of drive.
		Amount PriceDelta;

		/// Duration delta of drive.
		BlockDuration DurationDelta;

		/// Size delta of drive. TODO: After creation of drive we doesn't support it, so it must be zero
		uint64_t SizeDelta;

		/// Delta of replicas for drive.
		int8_t ReplicasDelta;

		/// Count of replicas for drive.
		int8_t MinReplicatorsDelta;

		/// Count of replicas for drive.
		int8_t MinApproversDelta;

	public:
		// Calculates the real size of a service \a transaction.
		static constexpr uint64_t CalculateRealSize(const TransactionType&) noexcept {
			return sizeof(TransactionType);
		}
	};

	DEFINE_EMBEDDABLE_TRANSACTION(ModifyDrive)

#pragma pack(pop)
}}
