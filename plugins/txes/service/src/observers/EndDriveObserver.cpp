/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "CommonDrive.h"

namespace catapult { namespace observers {

    using Notification = model::EndDriveNotification<1>;

    DECLARE_OBSERVER(EndDrive, Notification)(const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder) {
        return MAKE_OBSERVER(EndDrive, Notification, [pConfigHolder](const Notification& notification, ObserverContext& context) {
            auto& driveCache = context.Cache.sub<cache::DriveCache>();
            auto driveIter = driveCache.find(notification.DriveKey);
            auto& driveEntry = driveIter.get();
            auto streamingMosaicId = pConfigHolder->Config(context.Height).Immutable.StreamingMosaicId;
            auto storageMosaicId = pConfigHolder->Config(context.Height).Immutable.StorageMosaicId;
            auto currencyMosaicId = pConfigHolder->Config(context.Height).Immutable.CurrencyMosaicId;

            auto& accountStateCache = context.Cache.sub<cache::AccountStateCache>();
            auto accountIter = accountStateCache.find(driveEntry.key());
            auto& driveAccount = accountIter.get();
            auto ownerIter = accountStateCache.find(driveEntry.owner());
            auto& ownerAccount = ownerIter.get();

            if (NotifyMode::Commit == context.Mode) {
                if (driveEntry.state() == state::DriveState::InProgress) {
                    driveEntry.billingHistory().back().End = context.Height;
                }
                DrivePayment(driveEntry, context, storageMosaicId, {});

                for (auto& replicatorPair : driveEntry.replicators()) {
                    auto replicatorIter = accountStateCache.find(replicatorPair.first);
                    auto& replicatorAccount = replicatorIter.get();

                    for (const auto& filePair : driveEntry.files()) {
                        if (replicatorPair.second.ActiveFilesWithoutDeposit.count(filePair.first)) {
                            replicatorPair.second.ActiveFilesWithoutDeposit.erase(filePair.first);
                            replicatorPair.second.AddInactiveUndepositedFile(filePair.first, context.Height);
                        } else {
                            Credit(replicatorAccount, streamingMosaicId, utils::CalculateFileDeposit(driveEntry, filePair.first), context);
                        }
                    }

                    Credit(replicatorAccount, storageMosaicId, utils::CalculateDriveDeposit(driveEntry), context);
                }

                SetDriveState(driveEntry, context, state::DriveState::Finished);

                // We can return remaining xpx to user
                auto remainingCurrency = driveAccount.Balances.get(currencyMosaicId);

                // We use this payments array for streaming payments, but in case of removed drive we can store the last payment in xpx for drive
                driveEntry.uploadPayments().emplace_back(state::PaymentInformation{
                        driveEntry.owner(),
                        remainingCurrency,
                        context.Height
                });

                if (remainingCurrency > Amount(0))
                    Transfer(driveAccount, ownerAccount, currencyMosaicId, remainingCurrency, context);

                // If streaming amount is zero, it means that DriveFilesReward transaction will not force remove of drive.
                // So we need to do it by self
                if (driveAccount.Balances.get(streamingMosaicId) == Amount(0)) {
                    driveCache.markRemoveDrive(driveEntry.key(), context.Height);
                    driveEntry.setEnd(context.Height);
	                RemoveDriveMultisig(driveEntry, context);
                }
            } else {
                if (driveAccount.Balances.get(streamingMosaicId) == Amount(0)) {
                    driveCache.unmarkRemoveDrive(driveEntry.key(), context.Height);
                    driveEntry.setEnd(Height(0));
	                RemoveDriveMultisig(driveEntry, context);
                }

                if (driveEntry.uploadPayments().empty()
                    || driveEntry.uploadPayments().back().Receiver != driveEntry.owner()
                    || driveEntry.uploadPayments().back().Height != context.Height)
                    CATAPULT_THROW_RUNTIME_ERROR("rollback owner payment during finished drive");

                auto remainingCurrency = driveEntry.uploadPayments().back().Amount;
                driveEntry.uploadPayments().pop_back();

                if (remainingCurrency > Amount(0))
                    Transfer(ownerAccount, driveAccount, currencyMosaicId, remainingCurrency, context);

                for (auto& replicatorPair : driveEntry.replicators()) {
                    auto replicatorIter = accountStateCache.find(replicatorPair.first);
                    auto& replicatorAccount = replicatorIter.get();

                    for (const auto& filePair : driveEntry.files()) {
                        if (replicatorPair.second.InactiveFilesWithoutDeposit.count(filePair.first)
                            && replicatorPair.second.InactiveFilesWithoutDeposit.at(filePair.first).back() == context.Height) {
                            replicatorPair.second.ActiveFilesWithoutDeposit.insert(filePair.first);
                            replicatorPair.second.RemoveInactiveUndepositedFile(filePair.first, context.Height);
                        } else {
                            Debit(replicatorAccount, streamingMosaicId, utils::CalculateFileDeposit(driveEntry, filePair.first), context);
                        }
                    }

                    Debit(replicatorAccount, storageMosaicId, utils::CalculateDriveDeposit(driveEntry), context);
                }

                DrivePayment(driveEntry, context, storageMosaicId, {});
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
