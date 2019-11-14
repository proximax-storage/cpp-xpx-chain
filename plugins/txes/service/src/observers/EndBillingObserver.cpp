/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/observers/ObserverUtils.h"
#include "CommonDrive.h"

namespace catapult { namespace observers {

	using Notification = model::BlockNotification<1>;

	DECLARE_OBSERVER(EndBilling, Notification)(const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder) {
		return MAKE_OBSERVER(EndBilling, Notification, [pConfigHolder](const Notification&, ObserverContext& context) {
			auto& driveCache = context.Cache.sub<cache::DriveCache>();
			auto storageMosaicId = pConfigHolder->Config(context.Height).Immutable.StorageMosaicId;

			driveCache.processEndingDrives(context.Height, [&storageMosaicId, &context](state::DriveEntry& driveEntry) {
				if (driveEntry.state() != state::DriveState::InProgress)
					return;

			    DrivePayment(driveEntry, context, storageMosaicId);

                if (NotifyMode::Commit == context.Mode)
                    SetDriveState(driveEntry, context, state::DriveState::Pending);
                else
                    SetDriveState(driveEntry, context, state::DriveState::InProgress);
			});
		});
	};
}}
