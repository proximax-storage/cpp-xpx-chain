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
            for (auto j = 0; j < notification.VerificationOpinionsCount; ++j) {
                if (notification.ProversPtr[i] == notification.VerificationOpinionsPtr[j].Verifier)
                    continue;

                auto it = std::find_if(
                        notification.VerificationOpinionsPtr[j].Opinions.begin(),
                        notification.VerificationOpinionsPtr[j].Opinions.end(),
                        [&notification, i](const std::pair<Key, uint8_t>& el){return el.first == notification.ProversPtr[i];}
                );

                opinions += it->second;
            }

            // Check opinions` median
            if (opinions > notification.VerificationOpinionsCount / 2) {
                pendingVerification.Results.find(notification.ProversPtr[i])->second = 1;
                continue;
            }
            pendingVerification.Results.find(notification.ProversPtr[i])->second = 0;

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