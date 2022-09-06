/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"

namespace catapult { namespace observers {

	using Notification = model::DataModificationCancelNotification<1>;

	void DataModificationCancelHandler(const LiquidityProviderExchangeObserver& liquidityProvider, const Notification& notification, ObserverContext& context) {
		if (NotifyMode::Rollback == context.Mode)
			CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (DataModificationCancel)");

		auto& driveCache = context.Cache.sub<cache::BcDriveCache>();
		auto& driveEntry = driveCache.find(notification.DriveKey).get();

		auto& activeDataModifications = driveEntry.activeDataModifications();
		auto cancelingDataModificationIter = std::find_if(
				activeDataModifications.begin(),
				activeDataModifications.end(),
				[&notification](state::ActiveDataModification& modification) {
					return modification.Id == notification.DataModificationId;
				});

		// Making payments:
		const auto& modificationSize = cancelingDataModificationIter->ExpectedUploadSizeMegabytes;
		if (cancelingDataModificationIter->Id == activeDataModifications.front().Id) {
			// Performing download & upload work transfer for replicators
			const auto& replicators = driveEntry.replicators();
			if (!replicators.empty()) {
				const auto totalReplicatorAmount = Amount(
						modificationSize +    // Download work
						modificationSize * (replicators.size() - 1) / replicators.size());    // Upload work

				for (const auto& replicatorKey : replicators) {
					liquidityProvider.debitMosaics(
							context,
							driveEntry.key(),
							replicatorKey,
							config::GetUnresolvedStreamingMosaicId(context.Config.Immutable),
							totalReplicatorAmount);
				}
			}

			// Performing upload work transfer & stream refund for the drive owner
			const auto totalDriveOwnerAmount = Amount(
					modificationSize +    // Upload work
					2 * modificationSize *
							(driveEntry.replicatorCount() - replicators.size()));    // Stream refund

			liquidityProvider.debitMosaics(context, driveEntry.key(), driveEntry.owner(), config::GetUnresolvedStreamingMosaicId(context.Config.Immutable), totalDriveOwnerAmount);
		} else {
			// Performing refund for the drive owner
			const auto refundAmount = Amount(2 * modificationSize * driveEntry.replicatorCount());
			liquidityProvider.debitMosaics(context, driveEntry.key(), driveEntry.owner(), config::GetUnresolvedStreamingMosaicId(context.Config.Immutable), refundAmount);
		}

		// Updating data modification lists in the drive entry:
		driveEntry.completedDataModifications().emplace_back(state::CompletedDataModification{
				*cancelingDataModificationIter,
				state::DataModificationState::Cancelled
		});
		activeDataModifications.erase(cancelingDataModificationIter);
	}

	DEFINE_OBSERVER_WITH_LIQUIDITY_PROVIDER(DataModificationCancel, model::DataModificationCancelNotification<1>, [&liquidityProvider](const Notification& notification, ObserverContext& context) {
		DataModificationCancelHandler(liquidityProvider, notification, context);
    });
}}
