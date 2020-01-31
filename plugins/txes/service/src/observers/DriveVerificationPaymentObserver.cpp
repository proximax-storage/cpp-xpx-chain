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

			auto pFailedReplicator = notification.FailedReplicatorsPtr;
			std::vector<Key> faultyReplicatorKeys;
			faultyReplicatorKeys.reserve(notification.FailureCount);
			for (auto i = 0u; i < notification.FailureCount; ++i, ++pFailedReplicator) {
				faultyReplicatorKeys.emplace_back(*pFailedReplicator);
			}

            if (NotifyMode::Commit == context.Mode) {
                pFailedReplicator = notification.FailedReplicatorsPtr;
                for (auto i = 0u; i < notification.FailureCount; ++i, ++pFailedReplicator) {
                    // Set end eight for correct payment calculation. Than the replicator
                    // will move to the array of removed replicators.
                    driveEntry.replicators().at(*pFailedReplicator).End = context.Height;

                    // Add the replicator deposit to the drive account.
                    Credit(driveAccount, storageMosaicId, utils::CalculateDriveDeposit(driveEntry), context);
                }
            } else if (NotifyMode::Rollback == context.Mode) {
                for (pFailedReplicator = notification.FailedReplicatorsPtr + notification.FailureCount - 1; pFailedReplicator >= notification.FailedReplicatorsPtr; --pFailedReplicator) {
					driveEntry.restoreReplicator(*pFailedReplicator);
                }
            }

			DrivePayment(driveEntry, context, storageMosaicId, faultyReplicatorKeys);

            if (NotifyMode::Rollback == context.Mode) {
                pFailedReplicator = notification.FailedReplicatorsPtr;
                for (auto i = 0u; i < notification.FailureCount; ++i, ++pFailedReplicator) {
                    driveEntry.replicators().at(*pFailedReplicator).End = Height(0);

                    // Remove the replicator deposit from the drive account.
                    Debit(driveAccount, storageMosaicId, utils::CalculateDriveDeposit(driveEntry), context);
                }
            } else if (NotifyMode::Commit == context.Mode) {
                pFailedReplicator = notification.FailedReplicatorsPtr;
                for (auto i = 0u; i < notification.FailureCount; ++i, ++pFailedReplicator) {
                    driveEntry.removeReplicator(*pFailedReplicator);
                }
            }

			UpdateDriveMultisigSettings(driveEntry, context);
		})
	}
}}
