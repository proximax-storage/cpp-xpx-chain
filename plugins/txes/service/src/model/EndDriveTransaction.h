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

	/// Binary layout for a end drive transaction body.
	template<typename THeader>
	struct EndDriveTransactionBody : public THeader {
	private:
		using TransactionType = EndDriveTransactionBody<THeader>;

	public:
		DEFINE_TRANSACTION_CONSTANTS(Entity_Type_EndDrive, 1)

	public:
		/// Key of drive.
		Key DriveKey;

	public:
		// Calculates the real size of a service \a transaction.
		static constexpr uint64_t CalculateRealSize(const TransactionType&) noexcept {
			return sizeof(TransactionType);
		}
	};

	DEFINE_EMBEDDABLE_TRANSACTION(EndDrive)

#pragma pack(pop)
}}
