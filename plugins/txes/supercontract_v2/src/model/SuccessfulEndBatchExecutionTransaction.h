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
	struct AutomaticExecutionsPaymentTransactionBody : public THeader {
	private:
		using TransactionType = AutomaticExecutionsPaymentTransactionBody<THeader>;

	public:
		DEFINE_TRANSACTION_CONSTANTS(Entity_Type_AutomaticExecutionsPaymentTransaction, 1)

		/// Contract public key
		Key ContractKey;

		/// The number of prepaid automatic executions. Can be increased via special transaction.
		uint32_t AutomaticExecutionsNumber;

	public:
		// Calculates the real size of a deploy \a transaction.
		static constexpr uint64_t CalculateRealSize(const TransactionType& transaction) noexcept {
			return sizeof(TransactionType);
		}
	};

	DEFINE_EMBEDDABLE_TRANSACTION(AutomaticExecutionsPayment)

#pragma pack(pop)
}/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "SuperContractEntityType.h"
#include "catapult/model/Mosaic.h"
#include "catapult/model/Transaction.h"
#include <array>

namespace catapult::model {

#pragma pack(push, 1)

	struct CallDigest {
		Hash256 CallId;
		bool Success;
		Hash256 ReleasedTransactionHash;
	};

	struct CallPayment {
		Amount ExecutionPayment;
		Amount DownloadPayment;
	};

	using ProofOfExecutionPlaceHolder = std::array<uint8_t, 64>;

	/// Binary layout for a deploy transaction body.
	template<typename THeader>
	struct SuccessfulEndBatchExecutionTransactionBody : public THeader {
	private:
		using TransactionType = SuccessfulEndBatchExecutionTransactionBody<THeader>;

	public:
		DEFINE_TRANSACTION_CONSTANTS(Entity_Type_SuccessfulEndBatchExecutionTransaction, 1)

		/// Contract public key
		Key ContractKey;

		uint64_t BatchId;

		Hash256 StorageHash;

		std::array<uint8_t, 32> ProofOfExecutionVerificationInformation;

		uint16_t CosignersNumber;

		uint16_t CallsNumber;

		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(PublicKeys, Key)

		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(Signatures, Signature)

		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(ProofsOfExecution, ProofOfExecutionPlaceHolder)

		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(CallDigests, CallDigest)

		// Matrix of call payments. Rows - executors, columns - calls
		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(CallPayments, CallPayment)

	private:

		template<typename T>
		static auto* PublicKeysPtrT(T& transaction) {
			auto* pPayloadStart = THeader::PayloadStart(transaction);
			return transaction.CosignersNumber && pPayloadStart ? pPayloadStart : nullptr;
		}

		template<typename T>
		static auto* SignaturesPtrT(T& transaction) {
			auto* pPayloadStart = THeader::PayloadStart(transaction);
			return transaction.CosignersNumber && pPayloadStart
						   ? pPayloadStart
									 + transaction.CosignersNumber * sizeof(Key)
						   : nullptr;
		}

		template<typename T>
		static auto* ProofsOfExecutionPtrT(T& transaction) {
			auto* pPayloadStart = THeader::PayloadStart(transaction);
			return transaction.CosignersNumber && pPayloadStart
						   ? pPayloadStart
									 + transaction.CosignersNumber * sizeof(Key)
						   			 + transaction.CosignersNumber * sizeof(Signature)
						   : nullptr;
		}

		template<typename T>
		static auto* CallDigestsPtrT(T& transaction) {
			auto* pPayloadStart = THeader::PayloadStart(transaction);
			return transaction.CallsNumber && pPayloadStart
						   ? pPayloadStart
									 + transaction.CosignersNumber * sizeof(Key)
									 + transaction.CosignersNumber * sizeof(Signature)
									 + transaction.CosignersNumber * sizeof(ProofOfExecutionPlaceHolder)
						   : nullptr;
		}

		template<typename T>
		static auto* CallPaymentsPtrT(T& transaction) {
			auto* pPayloadStart = THeader::PayloadStart(transaction);
			return transaction.CosignersNumber && transaction.CallsNumber && pPayloadStart
						   ? pPayloadStart
									 + transaction.CosignersNumber * sizeof(Key)
									 + transaction.CosignersNumber * sizeof(Signature)
									 + transaction.CosignersNumber * sizeof(ProofOfExecutionPlaceHolder)
						   			 + transaction.CallsNumber * sizeof(CallDigest)
						   : nullptr;
		}

	public:
		// Calculates the real size of a deploy \a transaction.
		static constexpr uint64_t CalculateRealSize(const TransactionType& transaction) noexcept {
			return sizeof(TransactionType) +
				   transaction.CosignersNumber * sizeof(Key) +
				   transaction.CosignersNumber * sizeof(Signature) +
				   transaction.CosignersNumber * sizeof(ProofOfExecutionPlaceHolder) +
				   transaction.CallsNumber * sizeof(CallDigest) +
				   static_cast<uint64_t>(transaction.CosignersNumber) * static_cast<uint64_t>(transaction.CallsNumber) * sizeof(CallPayment);
		}
	};

	DEFINE_EMBEDDABLE_TRANSACTION(SuccessfulEndBatchExecution)

#pragma pack(pop)
}