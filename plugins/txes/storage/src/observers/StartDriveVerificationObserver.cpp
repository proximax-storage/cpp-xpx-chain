/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"

namespace catapult { namespace observers {

    DEFINE_OBSERVER(StartDriveVerification, model::StartDriveVerificationNotification<1>, ([](const model::StartDriveVerificationNotification<1>& notification, ObserverContext& context) {
        if (NotifyMode::Rollback == context.Mode)
            CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (StartDriveVerification)");

        const auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::StorageConfiguration>();
        auto verificationFrequency = pluginConfig.ExpectedVerificationFrequency.seconds() / notification.BlockGenerationTime.seconds();

        Key castedHash;
        std::copy(notification.BlockHash.begin(), notification.BlockHash.end(), castedHash.begin());

        uint64_t xorResult;
        std::memcpy(&xorResult, (notification.DriveKey ^ castedHash).data(), sizeof(uint64_t));

        // equal to (DriveKey ^ BlockHash) % pluginConfig.ExpectedVerificationFrequency
        if (xorResult & (verificationFrequency - 1) != 0)
            return;

        // Drive entry
        auto& driveCache = context.Cache.sub<cache::BcDriveCache>();
        auto driveIter = driveCache.find(notification.DriveKey);
        auto& driveEntry = driveIter.get();

        auto it = std::find_if(
                driveEntry.verifications().begin(),
                driveEntry.verifications().end(),
                [&notification](const state::Verification& v) {
                    return v.State == state::VerificationState::Pending;
                }
        );

        if (it != driveEntry.verifications().end())
            return;

        // Replicator cache
        auto& replicatorCache = context.Cache.sub<cache::ReplicatorCache>();

        // Add replicators that have the last state of the drive
        state::VerificationResults results;
        for (const auto& replicator: driveEntry.replicators()) {
            auto it = replicatorCache.find(replicator);
            auto replicatorEntry = it.tryGet();
            auto replicatorDrive = replicatorEntry->drives().find(driveEntry.key());
            if (replicatorDrive == replicatorEntry->drives().end())
                return;

            if (replicatorDrive->second.LastApprovedDataModificationId ==
                driveEntry.completedDataModifications().back().Id)
                results[replicator] = 0;
        }

        driveEntry.verifications().emplace_back(state::Verification{
                notification.BlockHash,
                state::VerificationState::Pending,
                results
        });
    }))
}}