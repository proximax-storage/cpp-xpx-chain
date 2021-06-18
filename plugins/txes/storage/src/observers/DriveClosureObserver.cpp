/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"

namespace catapult { namespace observers {

    DECLARE_OBSERVER(DriveClosure, model::DriveClosureNotification<1>)(const std::shared_ptr<cache::ReplicatorKeyCollector>& pKeyCollector) {
		return MAKE_OBSERVER(DriveClosure, model::DriveClosureNotification<1>, [pKeyCollector](const model::DriveClosureNotification<1>& notification, const ObserverContext& context) {
			if (NotifyMode::Rollback == context.Mode)
				CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (DriveClosure)");

			auto& driveCache = context.Cache.sub<cache::BcDriveCache>();
			driveCache.remove(notification.DriveKey);

			auto& replicatorCache = context.Cache.sub<cache::ReplicatorCache>();
			auto replicatorIter = replicatorCache.find(*pKeyCollector->keys().begin());
			auto& replicatorEntry = replicatorIter.get();
			replicatorEntry.drives().pop_back();
		})
	}
}}
