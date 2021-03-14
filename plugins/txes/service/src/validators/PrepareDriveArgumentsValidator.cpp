/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"

namespace catapult { namespace validators {

	namespace {

		template<VersionType version>
		ValidationResult handler_v1(const model::PrepareDriveNotification<version> &notification, const StatelessValidatorContext& context) {
			if (notification.Duration.unwrap() <= 0)
				return Failure_Service_Drive_Invalid_Duration;

			if (notification.BillingPeriod.unwrap() <= 0)
				return Failure_Service_Drive_Invalid_Billing_Period;

			if (notification.BillingPrice.unwrap() <= 0)
				return Failure_Service_Drive_Invalid_Billing_Price;

			if (notification.Duration.unwrap() % notification.BillingPeriod.unwrap() != 0)
				return Failure_Service_Drive_Duration_Is_Not_Multiple_Of_BillingPeriod;

			if (notification.DriveSize <= 0)
				return Failure_Service_Drive_Invalid_Size;

			if (notification.Replicas <= 0)
				return Failure_Service_Drive_Invalid_Replicas;

			if (notification.PercentApprovers > 100)
				return Failure_Service_Wrong_Percent_Approvers;

			if (notification.MinReplicators <= 0)
				return Failure_Service_Drive_Invalid_Min_Replicators;

			if (notification.MinReplicators > notification.Replicas)
				return Failure_Service_Min_Replicators_More_Than_Replicas;

			return ValidationResult::Success;
		}
	}

	DEFINE_STATELESS_VALIDATOR_WITH_TYPE(PrepareDriveArgumentsV1, model::PrepareDriveNotification<1>, handler_v1<1>)
	DEFINE_STATELESS_VALIDATOR_WITH_TYPE(PrepareDriveArgumentsV2, model::PrepareDriveNotification<2>, handler_v1<2>)
}}
