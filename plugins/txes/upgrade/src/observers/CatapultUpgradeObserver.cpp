/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/cache/CatapultUpgradeCache.h"

namespace catapult { namespace observers {

	using Notification = model::CatapultUpgradeNotification<1>;

	DECLARE_OBSERVER(CatapultUpgrade, Notification)() {
		return MAKE_OBSERVER(CatapultUpgrade, Notification, [config](const auto& notification, const ObserverContext& context) {
			auto& cache = context.Cache.sub<cache::CatapultUpgradeCache>();
			if (NotifyMode::Commit == context.Mode) {
				cache.insert(state::CatapultUpgradeEntry(notification.UpgradeHeight, notification.Version))
			} else {
				cache.remove(notification.UpgradeHeight);
			}
		});
	}
}}
