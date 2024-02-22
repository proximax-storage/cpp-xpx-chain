/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "DbrbEntityType.h"
#include "catapult/dbrb/DbrbDefinitions.h"
#include "catapult/model/Cosignature.h"
#include "catapult/model/Transaction.h"

namespace catapult { namespace model {

#pragma pack(push, 1)

	/// Binary layout for an add DBRB process transaction body.
	template<typename THeader>
	struct RemoveDbrbProcessByNetworkTransactionBody : public THeader {
	private:
		using TransactionType = RemoveDbrbProcessByNetworkTransactionBody<THeader>;

	public:
		DEFINE_TRANSACTION_CONSTANTS(Entity_Type_RemoveDbrbProcessByNetwork, 1)

	public:
		dbrb::ProcessId ProcessId;
		catapult::Timestamp Timestamp;
		uint16_t VoteCount;

		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(Votes, model::Cosignature)

	private:
		template<typename T>
		static auto* VotesPtrT(T& transaction) {
			return transaction.VoteCount ? THeader::PayloadStart(transaction) : nullptr;
		}

	public:
		// Calculates the real size of \a transaction.
		static constexpr uint64_t CalculateRealSize(const TransactionType& transaction) noexcept {
			return sizeof(TransactionType) + transaction.VoteCount * (Key_Size + Signature_Size);
		}
	};

	DEFINE_EMBEDDABLE_TRANSACTION(RemoveDbrbProcessByNetwork)

#pragma pack(pop)
}}
