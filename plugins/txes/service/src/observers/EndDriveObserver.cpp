/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "CommonDrive.h"

namespace catapult { namespace observers {

    using Notification = model::EndDriveNotification<1>;

    DECLARE_OBSERVER(EndDrive, Notification)(const config::ImmutableConfiguration& config) {
        return MAKE_OBSERVER(EndDrive, Notification, [&config](const Notification& notification, ObserverContext& context) {
            auto& driveCache = context.Cache.sub<cache::DriveCache>();
            auto driveIter = driveCache.find(notification.DriveKey);
            auto& driveEntry = driveIter.get();

            auto& accountStateCache = context.Cache.sub<cache::AccountStateCache>();

            if (NotifyMode::Commit == context.Mode) {
                if (driveEntry.state() == state::DriveState::InProgress) {
                    driveEntry.billingHistory().back().End = context.Height;
                }
                DrivePayment(driveEntry, context, config.StorageMosaicId, {});

                for (auto& replicatorPair : driveEntry.replicators()) {
                    auto replicatorIter = accountStateCache.find(replicatorPair.first);
                    auto& replicatorAccount = replicatorIter.get();

                    for (const auto& filePair : driveEntry.files()) {
                        if (replicatorPair.second.ActiveFilesWithoutDeposit.count(filePair.first)) {
                            replicatorPair.second.ActiveFilesWithoutDeposit.erase(filePair.first);
                            replicatorPair.second.AddInactiveUndepositedFile(filePair.first, context.Height);
                        } else {
                            Credit(replicatorAccount, config.StreamingMosaicId, utils::CalculateFileDeposit(filePair.second.Size), context);
                        }
                    }

                    Credit(replicatorAccount, config.StorageMosaicId, utils::CalculateDriveDeposit(driveEntry), context);
                }

                SetDriveState(driveEntry, context, state::DriveState::Finished);
            } else {
                for (auto& replicatorPair : driveEntry.replicators()) {
                    auto replicatorIter = accountStateCache.find(replicatorPair.first);
                    auto& replicatorAccount = replicatorIter.get();

                    for (const auto& filePair : driveEntry.files()) {
                        if (replicatorPair.second.InactiveFilesWithoutDeposit.count(filePair.first)
                            && replicatorPair.second.InactiveFilesWithoutDeposit.at(filePair.first).back() == context.Height) {
                            replicatorPair.second.ActiveFilesWithoutDeposit.insert(filePair.first);
                            replicatorPair.second.RemoveInactiveUndepositedFile(filePair.first, context.Height);
                        } else {
                            Debit(replicatorAccount, config.StreamingMosaicId, utils::CalculateFileDeposit(filePair.second.Size), context);
                        }
                    }

                    Debit(replicatorAccount, config.StorageMosaicId, utils::CalculateDriveDeposit(driveEntry), context);
                }

                DrivePayment(driveEntry, context, config.StorageMosaicId, {});
                auto expectedEnd = driveEntry.billingHistory().back().Start + Height(driveEntry.billingPeriod().unwrap());
                if (expectedEnd == driveEntry.billingHistory().back().End)
                    SetDriveState(driveEntry, context, state::DriveState::Pending);
                else {
                    driveEntry.billingHistory().back().End = expectedEnd;
                    SetDriveState(driveEntry, context, state::DriveState::InProgress);
                }
            }
        })
    };
}}
