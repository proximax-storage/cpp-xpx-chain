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

	DEFINE_STATEFUL_VALIDATOR(AggregateTransaction, [](const auto& notification, const auto& context) {
		if (notification.TransactionsCount && model::Entity_Type_OperationIdentify == notification.TransactionsPtr->Type) {
			const auto& operationCache = context.Cache.template sub<cache::OperationCache>();
			const auto& operationToken = static_cast<const model::EmbeddedOperationIdentifyTransaction&>(*notification.TransactionsPtr).OperationToken;
			if (Hash256() != operationToken && !operationCache.contains(operationToken))
				return Failure_Operation_Token_Invalid;
		}

		const auto* pTransaction = notification.TransactionsPtr;
		for (auto i = 1u; i < notification.TransactionsCount; ++i) {
			if (model::Entity_Type_OperationIdentify == pTransaction->Type)
				return Failure_Operation_Identify_Transaction_Misplaced;

			pTransaction = AdvanceNext(pTransaction);
		}

		return ValidationResult::Success;
	})
}}
