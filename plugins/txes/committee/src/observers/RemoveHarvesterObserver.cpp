/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/cache/CommitteeCache.h"

namespace catapult { namespace observers {

	DEFINE_OBSERVER(RemoveHarvester, model::RemoveHarvesterNotification<1>, [](const auto& notification, const ObserverContext& context) {
		if (NotifyMode::Rollback == context.Mode)
			CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (RemoveHarvester)");

		auto& cache = context.Cache.sub<cache::CommitteeCache>();
		cache.remove(notification.Signer);
	});
}}
