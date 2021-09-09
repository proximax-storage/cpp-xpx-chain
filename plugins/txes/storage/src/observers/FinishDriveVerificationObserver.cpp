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

        auto &driveCache = context.Cache.sub<cache::BcDriveCache>();
        auto driveIter = driveCache.find(notification.DriveKey);
        auto &driveEntry = driveIter.get();

        // Prover and median opinion about it`s result
        std::vector<std::pair<Key, uint8_t>> opinions;
        opinions.reserve(notification.ProversCount);

        // Find median for every Prover
        auto medianOpinion = 0;
        auto totalMedianOpinion = 0;
        for (auto i = 0; i < notification.ProversCount; ++i) {
            medianOpinion = 0;

            for (auto j = 0; j < notification.VerifiersOpinionsCount; ++j)
                medianOpinion += notification.VerifiersOpinionsPtr[j * notification.ProversCount];

            if (notification.VerifiersOpinionsCount / 2 < medianOpinion)
                medianOpinion = 1;
            else
                medianOpinion = 0;

            opinions.emplace_back(std::pair<Key, uint8_t>{notification.ProversPtr[i], medianOpinion});
            totalMedianOpinion += medianOpinion;
        }

        auto pendingVerification = driveEntry.verifications().back();
        pendingVerification.Opinions = opinions;
        if (notification.ProversCount / 2 < totalMedianOpinion)
            pendingVerification.State = state::VerificationState::Succeeded;
        else
            pendingVerification.State = state::VerificationState::Failed;
    }))
}}