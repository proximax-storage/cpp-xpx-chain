/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"

namespace catapult { namespace validators {

	using Notification = model::BlockNotification;

	DECLARE_STATELESS_VALIDATOR(Greed, Notification)() {
		return MAKE_STATELESS_VALIDATOR(MaxTransactions, [](const auto& notification) {
			if (notification.FeeInterestDenominator == 0)
				return Failure_Core_Invalid_FeeInterestDenominator;

			if (notification.FeeInterestDenominator < notification.FeeInterest)
				return Failure_Core_Invalid_FeeInterest;

			return ValidationResult::Success;
		});
	}
}}
