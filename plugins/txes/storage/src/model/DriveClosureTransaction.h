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

	/// Binary layout for a data modification cancel transaction body.
	template<typename THeader>
	struct DriveClosureTransactionBody : public THeader {
	private:
		using TransactionType = DriveClosureTransactionBody<THeader>;

	public:
		explicit DriveClosureTransactionBody<THeader>() {};

	public:
		DEFINE_TRANSACTION_CONSTANTS(Entity_Type_DriveClosure, 1)

	public:
		/// Key of a drive.
		Key DriveKey;

	public:
		// Calculates the real size of a storage \a transaction.
		static constexpr uint64_t CalculateRealSize(const TransactionType&) noexcept {
			return sizeof(TransactionType);
		}
	};

	DEFINE_EMBEDDABLE_TRANSACTION(DriveClosure)

#pragma pack(pop)
}}
