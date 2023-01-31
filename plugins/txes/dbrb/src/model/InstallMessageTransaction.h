/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "DbrbEntityType.h"
#include "catapult/dbrb/DbrbUtils.h"
#include "catapult/model/Cosignature.h"
#include "catapult/model/Transaction.h"

namespace catapult { namespace model {

#pragma pack(push, 1)

	/// Binary layout for an Install message transaction body.
	template<typename THeader>
	struct InstallMessageTransactionBody : public THeader {
	private:
		using TransactionType = InstallMessageTransactionBody<THeader>;

	public:
		DEFINE_TRANSACTION_CONSTANTS(Entity_Type_InstallMessage, 1)

	public:
		/// Install message hash.
		Hash256 MessageHash;

		/// Payload size in bytes.
		uint32_t PayloadSize;

		/// Payload - sequence and certificate.
		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(Payload, uint8_t)

	private:
		template<typename T>
		static auto* PayloadPtrT(T& transaction) {
			return THeader::PayloadStart(transaction);
		}

	public:
		// Calculates the real size of \a transaction.
		static constexpr uint64_t CalculateRealSize(const TransactionType& transaction) noexcept {
			return sizeof(TransactionType) + transaction.PayloadSize;
		}
	};

	DEFINE_EMBEDDABLE_TRANSACTION(InstallMessage)

#pragma pack(pop)
}}
