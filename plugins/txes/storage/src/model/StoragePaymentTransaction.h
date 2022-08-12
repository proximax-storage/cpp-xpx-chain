/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "StorageEntityType.h"
#include "catapult/model/Transaction.h"

namespace catapult { namespace model {

#pragma pack(push, 1)

	/// Binary layout for a storage payment transaction body.
	template<typename THeader>
	struct StoragePaymentTransactionBody : public THeader {
	private:
		using TransactionType = StoragePaymentTransactionBody<THeader>;

	public:
		explicit StoragePaymentTransactionBody<THeader>()
		{}

	public:
		DEFINE_TRANSACTION_CONSTANTS(Entity_Type_StoragePayment, 1)

	public:
		/// Key of the drive.
		Key DriveKey;

		/// Amount of storage units to transfer to the drive.
		Amount StorageUnits;

	public:
		// Calculates the real size of a storage \a transaction.
		static constexpr uint64_t CalculateRealSize(const TransactionType& transaction) noexcept {
			return sizeof(TransactionType);
		}
	};

	DEFINE_EMBEDDABLE_TRANSACTION(StoragePayment)

#pragma pack(pop)
}}
