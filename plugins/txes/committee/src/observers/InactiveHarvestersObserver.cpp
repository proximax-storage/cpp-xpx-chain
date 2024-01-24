/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/cache/CommitteeCache.h"
#include "catapult/cache/ReadOnlyCatapultCache.h"
#include "catapult/crypto/KeyPair.h"

namespace catapult { namespace observers {

	DEFINE_OBSERVER(InactiveHarvesters, model::InactiveHarvestersNotification<1>, [](const auto& notification, const ObserverContext& context) {
		if (NotifyMode::Rollback == context.Mode)
			CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (InactiveHarvesters)");

		auto& cache = context.Cache.sub<cache::CommitteeCache>();
		auto pHarvesterKey = notification.HarvesterKeysPtr;
		for (auto i = 0u; i < notification.HarvesterKeysCount; ++i, ++pHarvesterKey) {
			auto iter = cache.find(*pHarvesterKey);
			auto pEntry = iter.tryGet();
			if (pEntry) {
				pEntry->setExpirationTime(Timestamp(0));
			}
		}
	});
}}
