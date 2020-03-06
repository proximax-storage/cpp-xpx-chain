/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"

namespace catapult { namespace validators {

	using Notification = model::AggregateEmbeddedTransactionNotification<1>;

	DEFINE_STATELESS_VALIDATOR(EmbeddedTransaction, [](const Notification& notification) {
		if (notification.Index > 0) {
			auto type = notification.Transaction.Type;

			if (model::Entity_Type_OperationIdentify == type)
				return Failure_Operation_Identify_Transaction_Misplaced;

			if (model::Entity_Type_EndOperation == type)
				return Failure_Operation_End_Transaction_Misplaced;
		}

		return ValidationResult::Success;
	});
}}
