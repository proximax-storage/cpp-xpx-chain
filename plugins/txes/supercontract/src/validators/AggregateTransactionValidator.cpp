/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/model/SuperContractEntityType.h"
#include "plugins/txes/operation/src/validators/AggregateTransactionValidator.h"

namespace catapult { namespace validators {

	using Notification = model::AggregateCosignaturesNotification<1>;

	namespace {
		ValidationResult AggregateTransactionValidator(const Notification& notification) {
			return ValidateAggregateTransaction<
				model::Entity_Type_EndExecute,
				Failure_SuperContract_End_Execute_Transaction_Misplaced,
				Failure_SuperContract_Operation_Identify_Transaction_Misplaced,
				Failure_SuperContract_Operation_Identify_Transaction_Aggregated_With_End_Execute>(notification);
		}
	}

	DEFINE_STATELESS_VALIDATOR(AggregateTransaction, [](const auto& notification, const StatelessValidatorContext& context) {
		return AggregateTransactionValidator(notification);
	})
}}
