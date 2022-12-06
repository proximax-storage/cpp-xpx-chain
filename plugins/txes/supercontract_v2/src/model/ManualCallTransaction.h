/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "SuperContractEntityType.h"
#include "catapult/model/Mosaic.h"
#include "catapult/model/Transaction.h"

namespace catapult::model {

#pragma pack(push, 1)

	/// Binary layout for a deploy transaction body.
	template<typename THeader>
	struct ManualCallTransactionBody : public THeader {
	private:
		using TransactionType = ManualCallTransactionBody<THeader>;

	public:
		DEFINE_TRANSACTION_CONSTANTS(Entity_Type_ManualCallTransaction, 1)

		/// Contract public key on which the call is considered to be executed
		Key ContractKey;

		// region Params for StartExecuteTransaction (except for SuperContractKey).

		/// Message of FileName string in bytes
		uint16_t FileNameSize;

		/// Message of FunctionName string in bytes
		uint16_t FunctionNameSize;

		// Input parameters for this function string in bytes
		uint16_t ActualArgumentsSize;

		// Number of SC units provided to run the contract for all Executors on the Drive
		Amount ExecutionCallPayment;

		// Number of SM units provided to download data from the Internet for all Executors on the Drive
		Amount DownloadCallPayment;

		/// Number of necessary additional tokens to support specific Supercontract
		uint8_t ServicePaymentsCount;

		// endregion

		/// FileName to save stream in.
		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(FileName, uint8_t)

		/// FunctionName to save stream in.
		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(FunctionName, uint8_t)

		/// ActualArguments to save stream in.
		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(ActualArguments, uint8_t)

		// followed by ServicePayments data if ServicePaymentsCount != 0
		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(ServicePayments, UnresolvedMosaic)

	private:
		template<typename T>
		static auto* FileNamePtrT(T& transaction) {
			auto* pPayloadStart = THeader::PayloadStart(transaction);
			return transaction.FileNameSize && pPayloadStart ? pPayloadStart : nullptr;
		}

		template<typename T>
		static auto* FunctionNamePtrT(T& transaction) {
			return transaction.FunctionNameSize
			? THeader::PayloadStart(transaction)
			+ transaction.FileNameSize
			: nullptr;
		}

		template<typename T>
		static auto* ActualArgumentsPtrT(T& transaction) {
			auto* pPayloadStart = THeader::PayloadStart(transaction);
			return transaction.ActualArgumentsSize && pPayloadStart
			? pPayloadStart
			+ transaction.FileNameSize
			+ transaction.FunctionNameSize
			: nullptr;
		}

		template<typename T>
		static auto* ServicePaymentsPtrT(T& transaction) {
			auto* pPayloadStart = THeader::PayloadStart(transaction);
			return transaction.ServicePaymentsCount && pPayloadStart
			? pPayloadStart
			+ transaction.FileNameSize
			+ transaction.FunctionNameSize
			+ transaction.ActualArgumentsSize
			: nullptr;
		}

	public:
		// Calculates the real size of a manual call \a transaction.
		static constexpr uint64_t CalculateRealSize(const TransactionType& transaction) noexcept {
			return sizeof(TransactionType) + transaction.FileNameSize + transaction.FunctionNameSize + transaction.ActualArgumentsSize + transaction.ServicePaymentsCount * sizeof(UnresolvedMosaic);
		}
	};

	DEFINE_EMBEDDABLE_TRANSACTION(ManualCall)

#pragma pack(pop)
}