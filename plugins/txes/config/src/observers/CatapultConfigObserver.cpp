/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/cache/CatapultConfigCache.h"

namespace catapult { namespace observers {

	using Notification = model::BlockChainConfigNotification<1>;

	DECLARE_OBSERVER(CatapultConfig, Notification)() {
		return MAKE_OBSERVER(CatapultConfig, Notification, [](const auto& notification, const ObserverContext& context) {
			auto& cache = context.Cache.sub<cache::CatapultConfigCache>();
			auto height = Height{context.Height.unwrap() + notification.ApplyHeightDelta.unwrap()};
			if (NotifyMode::Commit == context.Mode) {
				cache.insert(state::CatapultConfigEntry(height, std::string((const char*)notification.BlockChainConfigPtr, notification.BlockChainConfigSize)));
			} else {
				cache.remove(height);
			}
		});
	}
}}
