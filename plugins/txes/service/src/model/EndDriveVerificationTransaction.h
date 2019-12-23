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

	/// Binary layout for an end drive verification transaction body.
	template<typename THeader>
	struct EndDriveVerificationTransactionBody : public THeader {
	private:
		using TransactionType = EndDriveVerificationTransactionBody<THeader>;

	public:
		DEFINE_TRANSACTION_CONSTANTS(Entity_Type_End_Drive_Verification, 1)

		/// Count of verification failures.
		uint16_t FailureCount;

		/// Verification failures.
		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(Failures, VerificationFailure)

	private:
		template<typename T>
		static auto* FailuresPtrT(T& transaction) {
			auto* pPayloadStart = THeader::PayloadStart(transaction);
			return transaction.FailureCount ? pPayloadStart : nullptr;
		}

	public:
		// Calculates the real size of a service \a transaction.
		static constexpr uint64_t CalculateRealSize(const TransactionType& transaction) noexcept {
			return sizeof(TransactionType) + transaction.FailureCount * sizeof(VerificationFailure);
		}
	};

	DEFINE_EMBEDDABLE_TRANSACTION(EndDriveVerification)

#pragma pack(pop)
}}
