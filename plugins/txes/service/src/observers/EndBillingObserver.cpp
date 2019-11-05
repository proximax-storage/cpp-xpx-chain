/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/cache/DriveCache.h"
#include "catapult/observers/ObserverUtils.h"
#include "src/utils/ServiceUtils.h"
#include "CommonDrive.h"
#include <cmath>

namespace catapult { namespace observers {

	using Notification = model::BlockNotification<1>;

	DECLARE_OBSERVER(EndBilling, Notification)(const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder) {
		return MAKE_OBSERVER(EndBilling, Notification, [pConfigHolder](const Notification&, const ObserverContext& context) {
			auto& driveCache = context.Cache.sub<cache::DriveCache>();
			auto storageMosaicId = pConfigHolder->Config(context.Height).Immutable.StorageMosaicId;

			driveCache.processMarkedDrives(context.Height, [&storageMosaicId, &context](state::DriveEntry& driveEntry) {
				if (driveEntry.state() != state::DriveState::InProgress)
					return;

			    DrivePayment(driveEntry, context, storageMosaicId);

                if (NotifyMode::Commit == context.Mode)
                    driveEntry.setState(state::DriveState::Pending);
                else
                    driveEntry.setState(state::DriveState::InProgress);
			});
		});
	};
}}
