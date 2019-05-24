/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "catapult/validators/ValidatorContext.h"
#include "catapult/version/version.h"
#include "plugins/txes/upgrade/src/cache/CatapultUpgradeCache.h"
#include "plugins/txes/upgrade/src/state/CatapultUpgradeEntry.h"

namespace catapult { namespace validators {

	using Notification = model::BlockNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(CatapultVersion, [](const auto&, const ValidatorContext& context) {
		const auto& cache = context.Cache.sub<cache::CatapultUpgradeCache>();
		const auto& entry = cache.find(context.Height).tryGet();
		if (entry && entry->catapultVersion() > version::CatapultVersion)
			return Failure_CatapultUpgrade_Invalid_Catapult_Version;

		return ValidationResult::Success;
	});
}}
