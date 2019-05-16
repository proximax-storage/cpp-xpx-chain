/**
*** Copyright 2018 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "ContractEntityType.h"
#include "catapult/model/Transaction.h"
#include "plugins/txes/multisig/src/model/ModifyMultisigAccountTransaction.h"

namespace catapult { namespace model {

#pragma pack(push, 1)

	/// Binary layout for a modify contract transaction body.
	template<typename THeader>
	struct ModifyContractTransactionBody : public THeader {
	private:
		using TransactionType = ModifyContractTransactionBody<THeader>;

	public:
		DEFINE_TRANSACTION_CONSTANTS(Entity_Type_Modify_Contract, 3)

	public:
		/// Relative change of the duration of the contract in blocks.
		int64_t DurationDelta;

		/// Hash of an entity passed from customers to executors (e.g. file hash).
		Hash256 Hash;

		/// Number of customer modifications.
		uint8_t CustomerModificationCount;

		/// Number of executor modifications.
		uint8_t ExecutorModificationCount;

		/// Number of verifier modifications.
		uint8_t VerifierModificationCount;

		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(CustomerModifications, CosignatoryModification)

		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(ExecutorModifications, CosignatoryModification)

		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(VerifierModifications, CosignatoryModification)

	private:
		template<typename T>
		static auto* CustomerModificationsPtrT(T& transaction) {
			return transaction.CustomerModificationCount ? THeader::PayloadStart(transaction) : nullptr;
		}

		template<typename T>
		static auto* ExecutorModificationsPtrT(T& transaction) {
			auto* pPayloadStart = THeader::PayloadStart(transaction);
			return transaction.ExecutorModificationCount && pPayloadStart ?
				pPayloadStart + transaction.CustomerModificationCount * sizeof(CosignatoryModification) : nullptr;
		}

		template<typename T>
		static auto* VerifierModificationsPtrT(T& transaction) {
			auto* pPayloadStart = THeader::PayloadStart(transaction);
			return transaction.VerifierModificationCount && pPayloadStart ? pPayloadStart
			   + transaction.CustomerModificationCount * sizeof(CosignatoryModification)
			   + transaction.ExecutorModificationCount * sizeof(CosignatoryModification)
			   : nullptr;
		}

	public:
		// Calculates the real size of a modify contract \a transaction.
		static constexpr uint64_t CalculateRealSize(const TransactionType& transaction) noexcept {
			auto size = sizeof(TransactionType)
				+ transaction.CustomerModificationCount * sizeof(CosignatoryModification)
				+ transaction.ExecutorModificationCount * sizeof(CosignatoryModification)
				+ transaction.VerifierModificationCount * sizeof(CosignatoryModification);
            return size;
		}
	};

	DEFINE_EMBEDDABLE_TRANSACTION(ModifyContract)

#pragma pack(pop)
}}
