/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"

namespace catapult { namespace observers {

	DEFINE_OBSERVER(DataModificationCancel,model::DataModificationCancelNotification<1>,[](const model::DataModificationCancelNotification<1>& notification, ObserverContext& context) {
		if (NotifyMode::Rollback == context.Mode)
			CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (DataModificationCancel)");

	  	auto& driveCache = context.Cache.sub<cache::BcDriveCache>();
		auto& driveEntry = driveCache.find(notification.DriveKey).get();

		auto& activeDataModifications = driveEntry.activeDataModifications();
        auto cancelingDataModificationIter = std::find_if(
                activeDataModifications.begin(),
                activeDataModifications.end(),
                [&notification](state::ActiveDataModification& modification){
                    return modification.Id == notification.DataModificationId;
		});

        const auto& currencyMosaicId = context.Config.Immutable.CurrencyMosaicId;
		const auto& streamingMosaicId = context.Config.Immutable.StreamingMosaicId;
		auto& accountStateCache = context.Cache.sub<cache::AccountStateCache>();
        auto driveStateIter = accountStateCache.find(notification.DriveKey);
        auto& driveState = driveStateIter.get();
		auto driveOwnerIter = accountStateCache.find(driveEntry.owner());
        auto& driveOwnerState = driveOwnerIter.get();

        // Making payments:
        const auto& modificationSize = cancelingDataModificationIter->ExpectedUploadSize;
		if (cancelingDataModificationIter->Id == activeDataModifications.front().Id) {
            // Performing download & upload work transfer for replicators
            const auto& replicators = driveEntry.replicators();
            const auto totalReplicatorAmount = Amount(
                    modificationSize +	// Download work
                    modificationSize * (replicators.size() - 1) / replicators.size());	// Upload work

            for (const auto& replicatorKey : replicators) {
                auto replicatorIter = accountStateCache.find(replicatorKey);
                auto& replicatorState = replicatorIter.get();
                driveState.Balances.debit(streamingMosaicId, totalReplicatorAmount, context.Height);
                replicatorState.Balances.credit(currencyMosaicId, totalReplicatorAmount, context.Height);
		    }

            // Performing upload work transfer & stream refund for the drive owner
            const auto totalDriveOwnerAmount = Amount(
                    modificationSize +	// Upload work
                    2 * modificationSize * (driveEntry.replicatorCount() - replicators.size()));	// Stream refund
            driveState.Balances.debit(streamingMosaicId, totalDriveOwnerAmount, context.Height);
            driveOwnerState.Balances.credit(currencyMosaicId, totalDriveOwnerAmount, context.Height);
        } else {
            // Performing refund for the drive owner
            const auto refundAmount = Amount(2 * modificationSize * driveEntry.replicatorCount());
            driveState.Balances.debit(streamingMosaicId, refundAmount, context.Height);
            driveOwnerState.Balances.credit(currencyMosaicId, refundAmount, context.Height);
		}

        // Updating data modification lists in the drive entry:
        driveEntry.completedDataModifications().emplace_back(state::CompletedDataModification{
                *cancelingDataModificationIter,
                state::DataModificationState::Cancelled
        });
        activeDataModifications.erase(cancelingDataModificationIter);
	})
}}
