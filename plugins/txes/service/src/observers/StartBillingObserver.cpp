/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "CommonDrive.h"

namespace catapult { namespace observers {

    using Notification = model::BalanceCreditNotification<1>;

    DECLARE_OBSERVER(StartBilling, Notification)(const MosaicId& storageMosaicId) {
        return MAKE_OBSERVER(StartBilling, Notification, [storageMosaicId](const Notification& notification, ObserverContext& context) {
            auto mosaicId = context.Resolvers.resolve(notification.MosaicId);

            if (storageMosaicId != mosaicId)
                return;

            auto& driveCache = context.Cache.sub<cache::DriveCache>();
            if (!driveCache.contains(notification.Sender))
                return;

            auto driveIter = driveCache.find(notification.Sender);
            auto& driveEntry = driveIter.get();

            auto driveBalance = utils::GetDriveBalance(driveEntry, context.Cache, mosaicId);

            if (NotifyMode::Commit == context.Mode) {
                if (driveEntry.state() == state::DriveState::Pending && driveBalance >= driveEntry.billingPrice()) {
                    SetDriveState(driveEntry, context, state::DriveState::InProgress);
                    state::BillingPeriodDescription period;
                    period.Start = context.Height;
                    period.End = period.Start + Height(driveEntry.billingPeriod().unwrap());
                    driveEntry.billingHistory().emplace_back(period);
                    driveCache.addBillingDrive(driveEntry.key(), driveEntry.billingHistory().back().End);
                }
            } else {
                if (driveEntry.state() == state::DriveState::InProgress && (driveBalance - notification.Amount) < driveEntry.billingPrice()) {
                    SetDriveState(driveEntry, context, state::DriveState::Pending);

                    driveCache.removeBillingDrive(driveEntry.key(), driveEntry.billingHistory().back().End);
                    if (driveEntry.billingHistory().back().Start != context.Height)
                        CATAPULT_THROW_RUNTIME_ERROR("Got unexpected state during drive billing rollback");

                    driveEntry.billingHistory().pop_back();
                }
            }
        })
    };
}}
