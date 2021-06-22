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

	/// Binary layout for a verification payment transaction body.
	template<typename THeader>
	struct VerificationPaymentTransactionBody : public THeader {
	private:
		using TransactionType = VerificationPaymentTransactionBody<THeader>;

	public:
		explicit VerificationPaymentTransactionBody<THeader>()
		{}

	public:
		DEFINE_TRANSACTION_CONSTANTS(Entity_Type_VerificationPayment, 1)

	public:
		/// Key of drive.
		Key DriveKey;

		/// Amount of XPXs to transfer to the drive.
		Amount VerificationFeeAmount;

	public:
		// Calculates the real size of a verification payment \a transaction.
		static constexpr uint64_t CalculateRealSize(const TransactionType& transaction) noexcept {
			return sizeof(TransactionType);
		}
	};

	DEFINE_EMBEDDABLE_TRANSACTION(VerificationPayment)

#pragma pack(pop)
}}
