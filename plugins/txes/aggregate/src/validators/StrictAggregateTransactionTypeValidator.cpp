/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/model/AggregateEntityType.h"

namespace catapult { namespace validators {

	using Notification = model::AggregateTransactionTypeNotification<2>;

	DECLARE_STATEFUL_VALIDATOR(StrictAggregateTransactionType, Notification)() {
		return MAKE_STATEFUL_VALIDATOR(StrictAggregateTransactionType, ([](const auto& notification, const auto& context) {
		   if (notification.Type == model::Entity_Type_Aggregate_Bonded)
				return Failure_Aggregate_Bonded_Not_Enabled;

			return ValidationResult::Success;
		}));
	}
}}
