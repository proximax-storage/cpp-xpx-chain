/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/cache/CommitteeCache.h"

namespace catapult { namespace validators {

	using Notification = model::RemoveHarvesterNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(RemoveHarvester, [](const auto& notification, const ValidatorContext& context) {
		auto& cache = context.Cache.sub<cache::CommitteeCache>();
		if (!cache.contains(notification.HarvesterKey))
			return Failure_Committee_Account_Does_Not_Exist;

		auto iter = cache.find(notification.HarvesterKey);
		const auto& entry = iter.get();

		if (entry.owner() != notification.Signer)
			return Failure_Committee_Signer_Is_Not_Owner;

		if (entry.disabledHeight() != Height(0))
			return Failure_Committee_Harvester_Already_Disabled;

		return ValidationResult::Success;
	});
}}
