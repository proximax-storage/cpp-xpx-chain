/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/cache/NetworkConfigCache.h"

namespace catapult { namespace observers {

	using Notification = model::NetworkConfigNotification<1>;

	DECLARE_OBSERVER(NetworkConfig, Notification)() {
		return MAKE_OBSERVER(NetworkConfig, Notification, [](const auto& notification, const ObserverContext& context) {
			auto& cache = context.Cache.sub<cache::NetworkConfigCache>();
			auto height = Height{context.Height.unwrap() + notification.ApplyHeightDelta.unwrap()};
			if (NotifyMode::Commit == context.Mode) {
				cache.insert(state::NetworkConfigEntry(height,
					std::string((const char*)notification.BlockChainConfigPtr, notification.BlockChainConfigSize),
					std::string((const char*)notification.SupportedEntityVersionsPtr, notification.SupportedEntityVersionsSize)));
			} else {
				if (cache.contains(height))
					cache.remove(height);
			}
		});
	}
}}
