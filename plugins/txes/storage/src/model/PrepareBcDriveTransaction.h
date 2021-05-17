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

	/// Binary layout for a prepare drive transaction body.
	template<typename THeader>
	struct PrepareBcDriveTransactionBody : public THeader {
	private:
		using TransactionType = PrepareBcDriveTransactionBody<THeader>;

	public:
		explicit PrepareBcDriveTransactionBody<THeader>()
		{}

	public:
		DEFINE_TRANSACTION_CONSTANTS(Entity_Type_PrepareBcDrive, 1)

	public:
		/// Size of drive.
		uint64_t DriveSize;

		/// Number of replicators.
		uint16_t ReplicatorCount;

	public:
		// Calculates the real size of a storage \a transaction.
		static constexpr uint64_t CalculateRealSize(const TransactionType&) noexcept {
			return sizeof(TransactionType);
		}
	};

	DEFINE_EMBEDDABLE_TRANSACTION(PrepareBcDrive)

#pragma pack(pop)
	}}
