/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/cache/DbrbViewCache.h"

namespace catapult { namespace observers {

	using Notification = model::RemoveDbrbProcessNotification<1>;

	DEFINE_OBSERVER(RemoveDbrbProcess, Notification, ([](const Notification& notification, const ObserverContext& context) {
		if (NotifyMode::Rollback == context.Mode)
			CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (RemoveDbrbProcess)");

		auto& cache = context.Cache.sub<cache::DbrbViewCache>();
		const auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::DbrbConfiguration>();
		if (pluginConfig.DbrbProcessLifetimeAfterExpiration.millis() > 0) {
			auto iter = cache.find(notification.ProcessId);
			auto pEntry = iter.tryGet();
			if (pEntry) {
				pEntry->setExpirationTime(context.Timestamp);
			}
		} else {
			if (cache.contains(notification.ProcessId))
				cache.remove(notification.ProcessId);
		}
	}))
}}
