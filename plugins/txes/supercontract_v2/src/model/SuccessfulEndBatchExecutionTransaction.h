/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "SuperContractEntityType.h"
#include "catapult/model/Mosaic.h"
#include "catapult/model/SupercontractModel.h"
#include "catapult/model/Transaction.h"
#include <array>

namespace catapult::model {

#pragma pack(push, 1)

	struct CallPayment {
		Amount ExecutionPayment;
		Amount DownloadPayment;
	};

	struct RawProofOfExecution {
		uint64_t StartBatchId;
		std::array<uint8_t, 32> T;
		std::array<uint8_t, 32> R;
		std::array<uint8_t, 32> F;
		std::array<uint8_t, 32> K;
	};

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

		uint64_t UsedSizeBytes;

		uint64_t MetaFilesSizeBytes;

		std::array<uint8_t, 32> ProofOfExecutionVerificationInformation;

		uint16_t CosignersNumber;

		uint16_t CallsNumber;

		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(PublicKeys, Key)

		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(Signatures, Signature)

		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(ProofsOfExecution, RawProofOfExecution)

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
									 + transaction.CosignersNumber * sizeof(RawProofOfExecution)
						   : nullptr;
		}

		template<typename T>
		static auto* CallPaymentsPtrT(T& transaction) {
			auto* pPayloadStart = THeader::PayloadStart(transaction);
			return transaction.CosignersNumber && transaction.CallsNumber && pPayloadStart
						   ? pPayloadStart
									 + transaction.CosignersNumber * sizeof(Key)
									 + transaction.CosignersNumber * sizeof(Signature)
									 + transaction.CosignersNumber * sizeof(RawProofOfExecution)
						   			 + transaction.CallsNumber * sizeof(CallDigest)
						   : nullptr;
		}

	public:
		// Calculates the real size of a deploy \a transaction.
		static constexpr uint64_t CalculateRealSize(const TransactionType& transaction) noexcept {
			return sizeof(TransactionType) +
				   transaction.CosignersNumber * sizeof(Key) +
				   transaction.CosignersNumber * sizeof(Signature) +
				   transaction.CosignersNumber * sizeof(RawProofOfExecution) +
				   transaction.CallsNumber * sizeof(CallDigest) +
				   static_cast<uint64_t>(transaction.CosignersNumber) * static_cast<uint64_t>(transaction.CallsNumber) * sizeof(CallPayment);
		}
	};

	DEFINE_EMBEDDABLE_TRANSACTION(SuccessfulEndBatchExecution)

#pragma pack(pop)
}