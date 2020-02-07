/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/cache/OperationCache.h"
#include "src/model/OperationIdentifyTransaction.h"

namespace catapult { namespace validators {

	using Notification = model::AggregateCosignaturesNotification<1>;

	namespace {
		const model::EmbeddedTransaction* AdvanceNext(const model::EmbeddedTransaction* pTransaction) {
			const auto* pTransactionData = reinterpret_cast<const uint8_t*>(pTransaction);
			return reinterpret_cast<const model::EmbeddedTransaction*>(pTransactionData + pTransaction->Size);
		}
	}

	DEFINE_STATELESS_VALIDATOR(AggregateTransaction, [](const auto& notification) {
		const auto* pTransaction = notification.TransactionsPtr;
		bool operationIdentifyPresent = (notification.TransactionsCount && model::Entity_Type_OperationIdentify == pTransaction->Type);
		for (auto i = 1u; i < notification.TransactionsCount; ++i) {
			if (model::Entity_Type_EndOperation == pTransaction->Type)
				return Failure_Operation_End_Transaction_Misplaced;
			pTransaction = AdvanceNext(pTransaction);
			if (model::Entity_Type_OperationIdentify == pTransaction->Type)
				return Failure_Operation_Identify_Transaction_Misplaced;
		}

		if (operationIdentifyPresent && model::Entity_Type_OperationIdentify == pTransaction->Type)
			return Failure_Operation_Identify_Transaction_Aggregated_With_End_Operation;

		return ValidationResult::Success;
	})
}}
