/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"

namespace catapult { namespace validators {

	using Notification = model::PrepareDriveNotification<1>;

	DEFINE_STATELESS_VALIDATOR(PrepareDriveArguments, [](const Notification& notification) {
			if (notification.Duration.unwrap() % notification.BillingPeriod.unwrap() != 0)
				return Failure_Service_Drive_Duration_Is_Not_Divide_On_BillingPeriod;

			if (notification.Duration.unwrap() <= 0)
				return Failure_Service_Drive_Invalid_Duration;

			if (notification.DriveSize <= 0)
				return Failure_Service_Drive_Invalid_Size;

			if (notification.PercentApprovers > 100)
				return Failure_Service_Wrong_Percent_Approvers;

			if (notification.MinReplicators > notification.Replicas)
				return Failure_Service_Min_Replicators_More_Than_Relicas;

			if (notification.MinReplicators <= 0)
				return Failure_Service_Drive_Invalid_Min_Replicators;

			if (notification.Replicas <= 0)
				return Failure_Service_Drive_Invalid_Replicas;

			return ValidationResult::Success;
		});
	}
}
