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

	DEFINE_STATEFUL_VALIDATOR(OperationIdentify, [](const auto& notification, const auto& context) {
		if (notification.TransactionsCount && model::Entity_Type_OperationIdentify == notification.TransactionsPtr->Type) {
			const auto& operationCache = context.Cache.template sub<cache::OperationCache>();
			const auto& operationToken = static_cast<const model::EmbeddedOperationIdentifyTransaction&>(*notification.TransactionsPtr).OperationToken;
			if (!operationCache.contains(operationToken))
				return Failure_Operation_Token_Invalid;
		}

		return ValidationResult::Success;
	})
}}
