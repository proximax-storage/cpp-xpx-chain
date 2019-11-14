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

	/// Binary layout for a start drive verification transaction body.
	template<typename THeader>
	struct StartDriveVerificationTransactionBody : public THeader {
	private:
		using TransactionType = StartDriveVerificationTransactionBody<THeader>;

	public:
		DEFINE_TRANSACTION_CONSTANTS(Entity_Type_Start_Drive_Verification, 1)

		/// Key of drive.
		Key DriveKey;

		/// Verification fee.
		Amount VerificationFee;

	public:
		// Calculates the real size of a service \a transaction.
		static constexpr uint64_t CalculateRealSize(const TransactionType&) noexcept {
			return sizeof(TransactionType);
		}
	};

	DEFINE_EMBEDDABLE_TRANSACTION(StartDriveVerification)

#pragma pack(pop)
}}
