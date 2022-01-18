/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"

namespace catapult { namespace observers {

    DEFINE_OBSERVER(EndDriveVerification, model::EndDriveVerificationNotification<1>, ([](const model::EndDriveVerificationNotification<1>& notification, ObserverContext& context) {
        if (NotifyMode::Rollback == context.Mode)
            CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (EndDriveVerification)");

        // Find median opinion for every Prover
		auto& replicatorCache = context.Cache.sub<cache::ReplicatorCache>();
		auto& driveCache = context.Cache.sub<cache::BcDriveCache>();
		auto storageDepositSlashing = 0;
        for (auto i = 0; i < notification.KeyCount; ++i) {
            uint8_t result = 0;
            for (auto j = 0; j < notification.JudgingKeyCount; ++j)
                result += notification.OpinionsPtr[i + j * (notification.KeyCount - 1)];

            if (result >= notification.JudgingKeyCount / 2)
                continue;

            auto replicatorIter = replicatorCache.find(notification.PublicKeysPtr[i]);
            auto& replicatorEntry = replicatorIter.get();

            // Count deposited Storage mosaics and delete the replicator from drives
			storageDepositSlashing += replicatorEntry.capacity().unwrap();
            for (const auto& pair: replicatorEntry.drives()) {
                auto driveIter = driveCache.find(pair.first);
                auto& drive = driveIter.get();

				storageDepositSlashing += drive.size();
                drive.replicators().erase(notification.PublicKeysPtr[i]);
            }

            replicatorCache.remove(notification.PublicKeysPtr[i]);
        }

		auto driveIter = driveCache.find(notification.DriveKey);
		auto& driveEntry = driveIter.get();

		const auto& replicatorKey = notification.PublicKeysPtr[0];
		auto& shards = driveEntry.verifications()[0].Shards;
		for (auto iter = shards.begin(); iter != shards.end(); ++iter) {
			const auto& shard = *iter;

			bool found = false;
			for (const auto& key : shard) {
				if (key == replicatorKey) {
					found = true;
					break;
				}
			}

			if (found) {
				shards.erase(iter);
				break;
			}
		}

		if (shards.empty())
			driveEntry.verifications().clear();

        if (storageDepositSlashing == 0)
            return;

        // Split storage deposit slashing between left replicators
        auto& accountStateCache = context.Cache.sub<cache::AccountStateCache>();
		auto accountIter = accountStateCache.find(notification.DriveKey);
		auto& driveAccountState = accountIter.get();
		auto storageDepositSlashingShare = Amount(storageDepositSlashing / driveEntry.replicators().size());
		const auto storageMosaicId = context.Config.Immutable.StorageMosaicId;

        for (const auto& replicatorKey : driveEntry.replicators()) {
            accountIter = accountStateCache.find(replicatorKey);
            auto& replicatorAccountState = accountIter.get();
			driveAccountState.Balances.debit(storageMosaicId, storageDepositSlashingShare, context.Height);
            replicatorAccountState.Balances.credit(storageMosaicId, storageDepositSlashingShare, context.Height);
        }
    }))
}}