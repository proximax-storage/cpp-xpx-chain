/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/cache/BlockchainUpgradeCache.h"

namespace catapult { namespace observers {

	using Notification = model::BlockchainUpgradeVersionNotification<1>;

	DECLARE_OBSERVER(BlockchainUpgrade, Notification)() {
		return MAKE_OBSERVER(BlockchainUpgrade, Notification, [](const auto& notification, const ObserverContext& context) {
			auto& cache = context.Cache.sub<cache::BlockchainUpgradeCache>();
			auto height = Height{context.Height.unwrap() + notification.UpgradePeriod.unwrap()};
			if (NotifyMode::Commit == context.Mode) {
				cache.insert(state::BlockchainUpgradeEntry(height, notification.Version));
			} else {
				if (cache.contains(height))
					cache.remove(height);
			}
		});
	}
}}
