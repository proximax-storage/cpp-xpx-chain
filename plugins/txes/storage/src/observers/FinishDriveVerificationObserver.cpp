/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"

namespace catapult { namespace observers {

    DEFINE_OBSERVER(FinishDriveVerification, model::FinishDriveVerificationNotification<1>,([](const model::FinishDriveVerificationNotification<1>& notification, ObserverContext& context) {
        if (NotifyMode::Rollback == context.Mode)
            CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (FinishDriveVerification)");

        //Mosaic ids
        const auto storageMosaicId = context.Config.Immutable.StorageMosaicId;
        const auto streamingMosaicId = context.Config.Immutable.StreamingMosaicId;

        // Replicator cache
        auto &replicatorCache = context.Cache.sub<cache::ReplicatorCache>();

        // Drive entry
        auto &driveCache = context.Cache.sub<cache::BcDriveCache>();
        auto driveIter = driveCache.find(notification.DriveKey);
        auto &driveEntry = driveIter.get();

        // Pending verification
        auto pendingVerification = driveEntry.verifications().back();

        auto opinions = 0;
        auto totalSplittingSoDeposit = 0;

        // Find median opinion for every Prover
        for (auto i = 0; i < notification.ProversCount; ++i) {
            opinions = 0;

            // Count all opinions
            for (auto j = 0; j < notification.VerifiersOpinionsCount; ++j)
                opinions += notification.VerifiersOpinionsPtr[j * notification.ProversCount];

            // If median greater than required check the next Prover
            if (notification.VerifiersOpinionsCount / 2 < opinions) {
                pendingVerification.Opinions.find(notification.ProversPtr[i])->second = 1;
                continue;
            }
            pendingVerification.Opinions.find(notification.ProversPtr[i])->second = 0;

            // Get replicator entry
            auto replicatorIter = replicatorCache.find(notification.ProversPtr[i]);
            auto &replicatorEntry = replicatorIter.get();

            // Count deposited Storage mosaics and delete the replicator from drives
            auto deposit = replicatorEntry.capacity().unwrap();
            for (const auto &iter: replicatorEntry.drives()) {
                auto driveIter = driveCache.find(iter.first);
                auto &drive = driveIter.get();

                deposit += drive.size();
                drive.replicators().erase(drive.replicators().find(notification.ProversPtr[i]));
            }
            totalSplittingSoDeposit += deposit;

            // Remove the replicator from the replicators cache and clear replicator`s drives
            replicatorCache.remove(notification.ProversPtr[i]);
            replicatorEntry.drives().clear();

            // Credit drive`s account for replicator`s Streaming deposit
            auto &cache = context.Cache.sub<cache::AccountStateCache>();
            auto accountIter = cache.find(notification.DriveKey);
            auto &driveState = accountIter.get();

            driveState.Balances.credit(streamingMosaicId, Amount(deposit * 2), context.Height);
        }

        // Split failed replicators` Storage deposits between left replicators
        auto &cache = context.Cache.sub<cache::AccountStateCache>();
        for (const auto &iter: driveEntry.replicators()) {
            auto accountIter = cache.find(iter);
            auto &replicatorState = accountIter.get();

            replicatorState.Balances.credit(
                    storageMosaicId,
                    Amount(totalSplittingSoDeposit / driveEntry.replicatorCount()),
                    context.Height
            );
        }

        // If there are left Storage mosaics send them to the first replicator
        if (totalSplittingSoDeposit % driveEntry.replicatorCount() != 0) {
            auto accountIter = cache.find(*driveEntry.replicators().begin());
            auto &replicatorState = accountIter.get();

            replicatorState.Balances.credit(
                    storageMosaicId,
                    Amount(totalSplittingSoDeposit % driveEntry.replicatorCount()),
                    context.Height
            );
        }

        pendingVerification.State = state::VerificationState::Finished;
    }))
}}