/**
*** Copyright 2025 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "StreamingEntityType.h"
#include "catapult/model/Transaction.h"

namespace catapult { namespace model {

#pragma pack(push, 1)

	/// Binary layout for an update drive size transaction body.
	template<typename THeader>
	struct UpdateDriveSizeTransactionBody : public THeader {
	private:
		using TransactionType = UpdateDriveSizeTransactionBody<THeader>;

	public:
		explicit UpdateDriveSizeTransactionBody<THeader>()
		{}

	public:
		DEFINE_TRANSACTION_CONSTANTS(Entity_Type_UpdateDriveSize, 1)

	public:
		/// Key of a drive to update.
		Key DriveKey;

		/// New size of the drive.
		uint64_t NewDriveSize;

	public:
		/// Calculates the real size of a streaming \a transaction.
		static constexpr uint64_t CalculateRealSize(const TransactionType&) noexcept {
			return sizeof(TransactionType);
		}
	};

	DEFINE_EMBEDDABLE_TRANSACTION(UpdateDriveSize)

#pragma pack(pop)
}}
