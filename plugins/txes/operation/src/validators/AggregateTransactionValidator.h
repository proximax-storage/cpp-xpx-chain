/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/validators/ValidationResult.h"
#include "src/model/OperationIdentifyTransaction.h"
#include "plugins/txes/aggregate/src/model/AggregateNotifications.h"

namespace catapult { namespace validators {

	template<
	    model::EntityType endTransactionEntityType,
		ValidationResult failureEndTransactionMisplaced,
		ValidationResult failureIdentifyTransactionMisplaced,
		ValidationResult failureIdentifyTransactionAggregatedWithEndTransaction>
	ValidationResult ValidateAggregateTransaction(const model::AggregateCosignaturesNotification<1>& notification) {
		const auto* pTransaction = notification.TransactionsPtr;
		bool operationIdentifyPresent = (notification.TransactionsCount && model::Entity_Type_OperationIdentify == pTransaction->Type);
		for (auto i = 1u; i < notification.TransactionsCount; ++i) {
			if (endTransactionEntityType == pTransaction->Type)
				return failureEndTransactionMisplaced;
			pTransaction = model::AdvanceNext(pTransaction);
			if (model::Entity_Type_OperationIdentify == pTransaction->Type)
				return failureIdentifyTransactionMisplaced;
		}

		if (operationIdentifyPresent && endTransactionEntityType == pTransaction->Type)
			return failureIdentifyTransactionAggregatedWithEndTransaction;

		return ValidationResult::Success;
	}
}}
