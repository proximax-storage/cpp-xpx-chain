/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "AggregateTransactionValidator.h"

namespace catapult { namespace validators {

	using Notification = model::AggregateCosignaturesNotification<1>;

	namespace {
		ValidationResult AggregateTransactionValidator(const Notification& notification) {
			return ValidateAggregateTransaction<
				model::Entity_Type_EndOperation,
				Failure_Operation_End_Transaction_Misplaced,
				Failure_Operation_Identify_Transaction_Misplaced,
				Failure_Operation_Identify_Transaction_Aggregated_With_End_Operation>(notification);
		}
	}

	DEFINE_STATELESS_VALIDATOR(AggregateTransaction, [](const auto& notification, const StatelessValidatorContext& context) {
		return AggregateTransactionValidator(notification);
	})
}}
