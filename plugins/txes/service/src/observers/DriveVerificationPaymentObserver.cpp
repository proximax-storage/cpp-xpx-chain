/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "CommonDrive.h"

namespace catapult { namespace observers {

	using Notification = model::DriveVerificationPaymentNotification<1>;

	DECLARE_OBSERVER(DriveVerificationPayment, Notification)(const MosaicId& storageMosaicId) {
		return MAKE_OBSERVER(DriveVerificationPayment, Notification, [storageMosaicId](const auto& notification, ObserverContext& context) {
			if (!notification.FailureCount)
				return;
			
			auto& driveCache = context.Cache.sub<cache::DriveCache>();
			auto driveIter = driveCache.find(notification.DriveKey);
			state::DriveEntry& driveEntry = driveIter.get();

            auto& accountStateCache = context.Cache.sub<cache::AccountStateCache>();
            auto driveAccountIter = accountStateCache.find(driveEntry.key());
            auto& driveAccount = driveAccountIter.get();

			auto pFailure = notification.FailuresPtr;
			std::vector<Key> faultyReplicatorKeys(notification.FailureCount);
			for (auto i = 0u; i < notification.FailureCount; ++i, ++pFailure) {
				faultyReplicatorKeys.emplace_back(pFailure->Replicator);
			}

            if (NotifyMode::Commit == context.Mode) {
                pFailure = notification.FailuresPtr;
                for (auto i = 0u; i < notification.FailureCount; ++i, ++pFailure) {
                    // Set end eight for correct payment calculation. Than the replicator
                    // will move to the array of removed replicators.
                    driveEntry.replicators().at(pFailure->Replicator).End = context.Height;

                    // Add the replicator deposit to the drive account.
                    Credit(driveAccount, storageMosaicId, utils::CalculateDriveDeposit(driveEntry), context);
                }
            } else if (NotifyMode::Rollback == context.Mode) {
                for (pFailure = notification.FailuresPtr + notification.FailureCount - 1; pFailure != notification.FailuresPtr; --pFailure) {
					driveEntry.restoreReplicator(pFailure->Replicator);
                }
            }

			DrivePayment(driveEntry, context, storageMosaicId, faultyReplicatorKeys);

            if (NotifyMode::Rollback == context.Mode) {
                pFailure = notification.FailuresPtr;
                for (auto i = 0u; i < notification.FailureCount; ++i, ++pFailure) {
                    driveEntry.replicators().at(pFailure->Replicator).End = Height(0);

                    // Remove the replicator deposit from the drive account.
                    Debit(driveAccount, storageMosaicId, utils::CalculateDriveDeposit(driveEntry), context);
                }
            } else if (NotifyMode::Commit == context.Mode) {
                pFailure = notification.FailuresPtr;
                for (auto i = 0u; i < notification.FailureCount; ++i, ++pFailure) {
                    driveEntry.removeReplicator(pFailure->Replicator);
                }
            }

			UpdateDriveMultisigSettings(driveEntry, context);
		})
	}
}}
