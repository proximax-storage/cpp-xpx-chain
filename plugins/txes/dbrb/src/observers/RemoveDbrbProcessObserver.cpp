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
		if (cache.contains(notification.ProcessId))
			cache.remove(notification.ProcessId);
	}))
}}
