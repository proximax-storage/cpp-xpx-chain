/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/cache/CommitteeCache.h"
#include "catapult/cache/ReadOnlyCatapultCache.h"

namespace catapult { namespace observers {

	DEFINE_OBSERVER(ActiveHarvesters, model::ActiveHarvestersNotification<1>, [](const auto& notification, const ObserverContext& context) {
		if (NotifyMode::Rollback == context.Mode)
			CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (ActiveHarvesters)");

		auto& cache = context.Cache.sub<cache::CommitteeCache>();
		auto pHarvesterKey = notification.HarvesterKeysPtr;
		for (auto i = 0u; i < notification.HarvesterKeysCount; ++i, ++pHarvesterKey) {
			auto iter = cache.find(*pHarvesterKey);
			auto pEntry = iter.tryGet();
			if (pEntry) {
				const auto& config = context.Config.Network;
				pEntry->setExpirationTime(context.Timestamp + Timestamp(config.DbrbRegistrationDuration.millis() + config.DbrbRegistrationGracePeriod.millis()));
				pEntry->setVersion(2);
			}
		}
	});
}}
