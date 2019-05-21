/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"

namespace catapult { namespace validators {

	using Notification = model::CatapultUpgradeNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(CatapultUpgrade, [](const auto& notification, const ValidatorContext& context) {
		if (notification.Height.unwrap() <= context.Height.unwrap())
			return Failure_CatapultUpgrade_Invalid_Height;

		const auto& cache = context.Cache.sub<cache::CatapultUpgradeCache>();
		auto height = notification.IsHeightRelative ? context.Height + notification.Height : notification.Height;
		if (cache.find(height).tryGet())
			return Failure_CatapultUpgrade_Redundant;

		return ValidationResult::Success;
	});
}}
