/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "plugins/txes/service/src/cache/DriveCache.h"
#include "src/cache/SuperContractCache.h"

namespace catapult { namespace observers {

    using Notification = model::EndDriveNotification<1>;

	DEFINE_OBSERVER(EndDrive, Notification, ([](const auto& notification, const ObserverContext& context) {
		auto& superContractCache = context.Cache.sub<cache::SuperContractCache>();
		auto& driveCache = context.Cache.sub<cache::DriveCache>();
		auto driveIter = driveCache.find(notification.DriveKey);
		auto& driveEntry = driveIter.get();

		for (const auto& coowner : driveEntry.coowners()) {
			if (superContractCache.contains(coowner)) {
				auto superContractCacheIter = superContractCache.find(coowner);
				auto& superContractEntry = superContractCacheIter.get();
				if (NotifyMode::Commit == context.Mode) {
					if (state::SuperContractState::Active == superContractEntry.state()) {
						superContractEntry.setState(state::SuperContractState::DeactivatedByDriveEnd);
					}
				} else {
					if (state::SuperContractState::DeactivatedByDriveEnd == superContractEntry.state()) {
						superContractEntry.setState(state::SuperContractState::Active);
					}
				}
			}
		}
	}));
}}
