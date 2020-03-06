/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "plugins/txes/service/src/cache/DriveCache.h"
#include "plugins/txes/service/src/state/DriveEntry.h"

namespace catapult { namespace observers {

	DEFINE_OBSERVER(Deploy, model::DeployNotification<1>, [](const auto& notification, ObserverContext& context) {
		auto& superContractCache = context.Cache.sub<cache::SuperContractCache>();
		auto& driveCache = context.Cache.sub<cache::DriveCache>();
		auto driveIter = driveCache.find(notification.Drive);
		state::DriveEntry& driveEntry = driveIter.get();

		if (NotifyMode::Commit == context.Mode) {
			state::SuperContractEntry superContractEntry(notification.SuperContract);
			superContractEntry.setStart(context.Height);
			superContractEntry.setFileHash(notification.FileHash);
			superContractEntry.setMainDriveKey(notification.Drive);
			superContractEntry.setVmVersion(notification.VmVersion);
			superContractCache.insert(superContractEntry);

			driveEntry.coowners().insert(notification.SuperContract);
		} else {
			if (superContractCache.contains(notification.SuperContract))
				superContractCache.remove(notification.SuperContract);

			driveEntry.coowners().erase(notification.SuperContract);
		}
	});
}}
