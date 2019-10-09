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

	/// Binary layout for a join to drive transaction body.
	template<typename THeader>
	struct FilesDepositTransactionBody : public THeader {
	private:
		using TransactionType = FilesDepositTransactionBody<THeader>;

	public:
		DEFINE_TRANSACTION_CONSTANTS(Entity_Type_FilesDeposit, 1)

		/// Key of drive.
		Key DriveKey;

		/// Count of files.
		uint16_t FilesCount;

		/// Files for deposit.
		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(Files, File)

	private:
		template<typename T>
		static auto* FilesPtrT(T& transaction) {
			auto* pPayloadStart = THeader::PayloadStart(transaction);
			return pPayloadStart ? pPayloadStart : nullptr;
		}

	public:
		// Calculates the real size of a service \a transaction.
		static constexpr uint64_t CalculateRealSize(const TransactionType& transaction) noexcept {
			return sizeof(TransactionType) + transaction.FilesCount * sizeof(File);
		}
	};

	DEFINE_EMBEDDABLE_TRANSACTION(FilesDeposit)

#pragma pack(pop)
}}
