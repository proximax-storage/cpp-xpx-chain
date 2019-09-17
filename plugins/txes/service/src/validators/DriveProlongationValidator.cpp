/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "catapult/validators/ValidatorContext.h"

namespace catapult { namespace validators {

	using Notification = model::DriveProlongationNotification<1>;

	DEFINE_STATELESS_VALIDATOR(DriveProlongation, [](const auto& notification) {
		if (notification.Duration.unwrap() == 0)
			return Failure_Service_Drive_Invalid_Prolongation_Duration;

		return ValidationResult::Success;
	});
}}
