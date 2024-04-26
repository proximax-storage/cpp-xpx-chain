/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "StorageEntityType.h"
#include "catapult/model/Transaction.h"
#include "catapult/utils/ArraySet.h"

namespace catapult { namespace model {

#pragma pack(push, 1)

	/// Binary layout for a replicators cleanup transaction body.
	template<typename THeader>
	struct ReplicatorsCleanupTransactionBody : public THeader {
	private:
		using TransactionType = ReplicatorsCleanupTransactionBody<THeader>;

	public:
		DEFINE_TRANSACTION_CONSTANTS(Entity_Type_ReplicatorsCleanup, 1)

	public:
		/// The number of replicators to remove.
		uint16_t ReplicatorCount;

		/// Public keys of the replicators to remove.
		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(ReplicatorKeys, Key)

	private:
		template<typename T>
		static auto* ReplicatorKeysPtrT(T& transaction) {
			return transaction.ReplicatorCount ? THeader::PayloadStart(transaction) : nullptr;
		}

	public:
		// Calculates the real size of a replicators cleanup \a transaction.
		static constexpr uint64_t CalculateRealSize(const TransactionType& transaction) noexcept {
			return sizeof(TransactionType) + transaction.ReplicatorCount * Key_Size;
		}
	};

	DEFINE_EMBEDDABLE_TRANSACTION(ReplicatorsCleanup)

#pragma pack(pop)
}}
