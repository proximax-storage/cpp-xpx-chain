/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/cache/CatapultUpgradeCache.h"

namespace catapult { namespace observers {

	using Notification = model::CatapultUpgradeVersionNotification<1>;

	DECLARE_OBSERVER(CatapultUpgrade, Notification)() {
		return MAKE_OBSERVER(CatapultUpgrade, Notification, [](const auto& notification, const ObserverContext& context) {
			auto& cache = context.Cache.sub<cache::CatapultUpgradeCache>();
			auto height = Height{context.Height.unwrap() + notification.UpgradePeriod.unwrap()};
			if (NotifyMode::Commit == context.Mode) {
				cache.insert(state::CatapultUpgradeEntry(height, notification.Version));
			} else {
				if (cache.contains(height))
					cache.remove(height);
			}
		});
	}
}}
