/**
*** Copyright (c) 2018-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
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
