/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/model/SuperContractEntityType.h"

namespace catapult { namespace validators {

	using Notification = model::AggregateEmbeddedTransactionNotification<1>;

	DEFINE_STATELESS_VALIDATOR(EmbeddedTransaction, [](const Notification& notification) {
		if (notification.Index > 0 && model::Entity_Type_EndExecute == notification.Transaction.Type)
			return Failure_SuperContract_End_Execute_Transaction_Misplaced;

		return ValidationResult::Success;
	});
}}
