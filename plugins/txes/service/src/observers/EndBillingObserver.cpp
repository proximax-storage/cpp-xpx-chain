/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/observers/ObserverUtils.h"
#include "CommonDrive.h"

namespace catapult { namespace observers {

	using Notification = model::BlockNotification<1>;

	DECLARE_OBSERVER(EndBilling, Notification)(const MosaicId& storageMosaicId) {
		return MAKE_OBSERVER(EndBilling, Notification, [storageMosaicId](const Notification&, ObserverContext& context) {
			auto& driveCache = context.Cache.sub<cache::DriveCache>();

			driveCache.processBillingDrives(context.Height, [storageMosaicId, &context](state::DriveEntry& driveEntry) {
				auto expectedState = (NotifyMode::Commit == context.Mode) ? state::DriveState::InProgress : state::DriveState::Pending;
				if (driveEntry.state() != expectedState)
					return;

			    DrivePayment(driveEntry, context, storageMosaicId, {});

                if (NotifyMode::Commit == context.Mode)
                    SetDriveState(driveEntry, context, state::DriveState::Pending);
                else
                    SetDriveState(driveEntry, context, state::DriveState::InProgress);
			});
		});
	};
}}
