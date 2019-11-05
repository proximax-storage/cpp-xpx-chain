/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "src/cache/DriveCache.h"
#include "CommonDrive.h"

namespace catapult { namespace observers {

    using Notification = model::EndDriveNotification<1>;

    DECLARE_OBSERVER(EndDrive, Notification)(const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder) {
        return MAKE_OBSERVER(EndDrive, Notification, [pConfigHolder](const Notification& notification, const ObserverContext& context) {
            const auto& config = pConfigHolder->Config(context.Height).Immutable;

            auto& driveCache = context.Cache.sub<cache::DriveCache>();
            auto driveIter = driveCache.find(notification.DriveKey);
            auto& driveEntry = driveIter.get();

            auto& accountStateCache = context.Cache.sub<cache::AccountStateCache>();
            auto accountIter = accountStateCache.find(driveEntry.key());
            auto& driveAccount = accountIter.get();

            if (NotifyMode::Commit == context.Mode) {
                if (driveEntry.state() == state::DriveState::InProgress) {
                    driveEntry.billingHistory().back().End = context.Height;
                }
                DrivePayment(driveEntry, context, config.StorageMosaicId);

                for (const auto& replicatorPair : driveEntry.replicators()) {
                    auto replicatorIter = accountStateCache.find(replicatorPair.first);
                    auto& replicatorAccount = replicatorIter.get();

                    Transfer(driveAccount, replicatorAccount, config.StorageMosaicId, replicatorPair.second.Deposit, context.Height);
                }

                for (auto& filePair : driveEntry.files()) {
                    state::FileInfo& info = filePair.second;
                    if (info.Actions.back().Type != state::DriveActionType::Remove || info.Actions.back().ActionHeight == context.Height)
                        info.Actions.emplace_back(state::DriveAction{ state::DriveActionType::Remove, context.Height });
                }

                driveEntry.setState(state::DriveState::Finished);
            } else {
                for (auto& filePair : driveEntry.files()) {
                    state::FileInfo& info = filePair.second;
                    if (info.Actions.back().Type == state::DriveActionType::Remove && info.Actions.back().ActionHeight == context.Height)
                        info.Actions.pop_back();
                }

                for (const auto& replicatorPair : driveEntry.replicators()) {
                    auto replicatorIter = accountStateCache.find(replicatorPair.first);
                    auto& replicatorAccount = replicatorIter.get();

                    Transfer(replicatorAccount, driveAccount, config.StorageMosaicId, replicatorPair.second.Deposit, context.Height);
                }

                DrivePayment(driveEntry, context, config.StorageMosaicId);
                auto expectedEnd = driveEntry.billingHistory().back().Start + Height(driveEntry.billingPeriod().unwrap());
                if (expectedEnd == driveEntry.billingHistory().back().End)
                    driveEntry.setState(state::DriveState::Pending);
                else {
                    driveEntry.billingHistory().back().End = expectedEnd;
                    driveEntry.setState(state::DriveState::InProgress);
                }
            }
        })
    };
}}
