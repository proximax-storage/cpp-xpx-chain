/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/cache/CommitteeCache.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "catapult/cache_core/ImportanceView.h"

namespace catapult { namespace validators {

	using Notification = model::AddHarvesterNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(AddHarvester, [](const auto& notification, const ValidatorContext& context) {
		auto& cache = context.Cache.sub<cache::CommitteeCache>();
		if (cache.contains(notification.Signer))
			return Failure_Committee_Redundant;

		cache::ImportanceView view(context.Cache.sub<cache::AccountStateCache>());
		if (!view.canHarvest(notification.Signer, context.Height, context.Config.Network.MinHarvesterBalance))
			return Failure_Committee_Harvester_Ineligible;

		return ValidationResult::Success;
	});
}}
