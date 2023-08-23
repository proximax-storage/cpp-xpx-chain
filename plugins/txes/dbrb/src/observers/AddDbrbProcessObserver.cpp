/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/cache/DbrbViewCache.h"

namespace catapult { namespace observers {

	using Notification = model::AddDbrbProcessNotification<1>;

	DECLARE_OBSERVER(AddDbrbProcess, Notification)() {
		return MAKE_OBSERVER(AddDbrbProcess, Notification, ([](const Notification& notification, const ObserverContext& context) {
		  	if (NotifyMode::Rollback == context.Mode)
			  	CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (AddDbrbProcess)");

			const auto& config = context.Config.Network;
			auto expirationTime = context.Timestamp + Timestamp(config.DbrbRegistrationDuration.millis() + config.DbrbRegistrationGracePeriod.millis());

		  	auto& cache = context.Cache.sub<cache::DbrbViewCache>();
			if (cache.contains(notification.ProcessId)) {
				auto iter = cache.find(notification.ProcessId);
				auto& entry = iter.get();
				entry.setExpirationTime(expirationTime);
			} else {
				cache.insert(state::DbrbProcessEntry(notification.ProcessId, expirationTime));
			}
		}))
	}
}}
