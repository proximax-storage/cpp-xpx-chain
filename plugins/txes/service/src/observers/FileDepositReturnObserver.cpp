/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/cache/DriveCache.h"

namespace catapult { namespace observers {

	DEFINE_OBSERVER(FileDepositReturn, model::FileDepositReturnNotification<1>, [](const auto& notification, const ObserverContext& context) {
		auto& driveCache = context.Cache.sub<cache::DriveCache>();
		auto& driveEntry = driveCache.find(notification.Drive).get();
		auto& replicatorDepositMap = driveEntry.replicators()[notification.Replicator];
		if (NotifyMode::Commit == context.Mode) {
			replicatorDepositMap.erase(notification.FileHash);
		} else {
			replicatorDepositMap.emplace(notification.FileHash, notification.Deposit);
		}
	});
}}
