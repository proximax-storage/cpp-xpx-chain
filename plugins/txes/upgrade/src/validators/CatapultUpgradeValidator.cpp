/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "catapult/validators/ValidatorContext.h"
#include "src/cache/CatapultUpgradeCache.h"

namespace catapult { namespace validators {

	using Notification = model::CatapultUpgradeVersionNotification<1>;

	DECLARE_STATEFUL_VALIDATOR(CatapultUpgrade, Notification)(BlockDuration minUpgradePeriod) {
		return MAKE_STATEFUL_VALIDATOR(CatapultUpgrade, ([minUpgradePeriod](const Notification& notification, const ValidatorContext& context) {
			auto upgradePeriod = notification.UpgradePeriod.unwrap();
			if (minUpgradePeriod.unwrap() > upgradePeriod)
				return Failure_CatapultUpgrade_Upgrade_Period_Too_Low;

			const auto& cache = context.Cache.sub<cache::CatapultUpgradeCache>();
			auto height = Height{context.Height.unwrap() + upgradePeriod};
			if (cache.find(height).tryGet())
				return Failure_CatapultUpgrade_Redundant;

			return ValidationResult::Success;
		}));
	}
}}
