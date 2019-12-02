/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "ServiceEntityType.h"
#include "ServiceTypes.h"
#include "catapult/model/Transaction.h"
#include "catapult/model/TransactionContainer.h"

namespace catapult { namespace model {

#pragma pack(push, 1)

	/// Binary layout for an delete reward transaction header.
	template<typename THeader>
	struct DeleteRewardTransactionHeader : public THeader {
	public:
		DEFINE_TRANSACTION_CONSTANTS(Entity_Type_DeleteReward, 1)
	};

	/// Binary layout for a basic delete reward transaction body.
	template<typename THeader>
	struct DeleteRewardTransactionBody : public TransactionContainer<DeleteRewardTransactionHeader<THeader>, DeletedFile> {
	private:
		using TransactionType = DeleteRewardTransactionBody<THeader>;

	public:
		// Calculates the real size of delete reward \a transaction.
		static constexpr uint64_t CalculateRealSize(const TransactionType& transaction) noexcept {
			return IsSizeValid(transaction) ? transaction.Size : std::numeric_limits<uint64_t>::max();
		}

	private:
		static bool IsSizeValid(const TransactionType& transaction) {
			auto transactions = transaction.Transactions(EntityContainerErrorPolicy::Suppress);
			auto areAllTransactionsValid = std::all_of(transactions.cbegin(), transactions.cend(), [](const auto& modification) {
				return modification.IsSizeValid();
			});

			if (areAllTransactionsValid && !transactions.hasError())
				return true;

			CATAPULT_LOG(warning) << "delete reward transaction failed size validation (valid sizes? " << areAllTransactionsValid
								  << ", errors? " << transactions.hasError() << ")";
			return false;
		}
	};

	template<typename TransactionType>
	// Calculates the PayloadSize of metadata \a transaction.
	static constexpr size_t GetTransactionPayloadSize(const TransactionType& transaction) noexcept {
		return transaction.Size - sizeof(TransactionType);
	}

	DEFINE_EMBEDDABLE_TRANSACTION(DeleteReward)

#pragma pack(pop)
}}
