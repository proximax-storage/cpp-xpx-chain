/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "StreamingEntityType.h"
#include "catapult/model/Transaction.h"
#include "catapult/utils/ArraySet.h"

namespace catapult { namespace model {

#pragma pack(push, 1)

	/// Binary layout for a data modification transaction body.
	template<typename THeader>
	struct StreamPaymentTransactionBody : public THeader {
	private:
		using TransactionType = StreamPaymentTransactionBody<THeader>;

	public:
		explicit StreamPaymentTransactionBody<THeader>()
		{}

	public:
		DEFINE_TRANSACTION_CONSTANTS(Entity_Type_StreamPayment, 1)

	public:
		/// Key of drive.
		Key DriveKey;

		/// Id of stream to be finished
		Hash256 StreamId;

		/// Actual size of stream
		uint64_t AdditionalUploadSizeMegabytes;

	public:
		// Calculates the real size of a stream payment \a transaction.
		static constexpr uint64_t CalculateRealSize(const TransactionType& transaction) noexcept {
			return sizeof(TransactionType);
		}
	};

	DEFINE_EMBEDDABLE_TRANSACTION(StreamPayment)

#pragma pack(pop)
}}
