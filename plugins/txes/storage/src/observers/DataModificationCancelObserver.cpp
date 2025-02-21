/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"

namespace catapult { namespace observers {

	using Notification = model::DataModificationCancelNotification<1>;

	DEFINE_OBSERVER_WITH_LIQUIDITY_PROVIDER(DataModificationCancel, Notification, ([&liquidityProvider, pStorageState](const Notification& notification, ObserverContext& context) {
		if (NotifyMode::Rollback == context.Mode)
			CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (DataModificationCancel)");

		auto& driveCache = context.Cache.sub<cache::BcDriveCache>();
		auto driveIt = driveCache.find(notification.DriveKey);
		auto& driveEntry = driveIt.get();

	  	const auto& currencyMosaicId = context.Config.Immutable.CurrencyMosaicId;
		const auto& streamingMosaicId = context.Config.Immutable.StreamingMosaicId;
		auto& statementBuilder = context.StatementBuilder();

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
					liquidityProvider->debitMosaics(
							context,
							driveEntry.key(),
							replicatorKey,
							config::GetUnresolvedStreamingMosaicId(context.Config.Immutable),
							totalReplicatorAmount);

					// Adding Pending Replicator receipt.
					const auto receiptType = model::Receipt_Type_Data_Modification_Cancel_Pending_Replicator;
					const model::StorageReceipt receipt(receiptType, driveEntry.key(), replicatorKey,
														{ streamingMosaicId, currencyMosaicId }, totalReplicatorAmount);
					statementBuilder.addTransactionReceipt(receipt);
				}
			}

			// Performing upload work transfer & stream refund for the drive owner
			const auto totalDriveOwnerAmount = Amount(
					modificationSize +    // Upload work
					2 * modificationSize *
							(driveEntry.replicatorCount() - replicators.size()));    // Stream refund

			liquidityProvider->debitMosaics(context, driveEntry.key(), driveEntry.owner(), config::GetUnresolvedStreamingMosaicId(context.Config.Immutable), totalDriveOwnerAmount);

			// Adding Pending Owner receipt.
			const auto receiptType = model::Receipt_Type_Data_Modification_Cancel_Pending_Owner;
			const model::StorageReceipt receipt(receiptType, driveEntry.key(), driveEntry.owner(),
												{ streamingMosaicId, currencyMosaicId }, totalDriveOwnerAmount);
			statementBuilder.addTransactionReceipt(receipt);
		} else {
			// Performing refund for the drive owner
			const auto refundAmount = Amount(2 * modificationSize * driveEntry.replicatorCount());
			liquidityProvider->debitMosaics(context, driveEntry.key(), driveEntry.owner(), config::GetUnresolvedStreamingMosaicId(context.Config.Immutable), refundAmount);

			// Adding Queued receipt.
			const auto receiptType = model::Receipt_Type_Data_Modification_Cancel_Queued;
			const model::StorageReceipt receipt(receiptType, driveEntry.key(), driveEntry.owner(),
												{ streamingMosaicId, currencyMosaicId }, refundAmount);
			statementBuilder.addTransactionReceipt(receipt);
		}

		// Updating data modification lists in the drive entry:
		driveEntry.completedDataModifications().emplace_back(state::CompletedDataModification{
				*cancelingDataModificationIter,
				state::DataModificationApprovalState::Cancelled,
				0U
		});
		activeDataModifications.erase(cancelingDataModificationIter);

		const auto& replicators = driveEntry.replicators();
		if (replicators.find(pStorageState->replicatorKey()) == replicators.end())
			return;

		auto& replicatorCache = context.Cache.template sub<cache::ReplicatorCache>();
		auto& downloadChannelCache = context.Cache.template sub<cache::DownloadChannelCache>();
		auto pDrive = utils::GetDrive(notification.DriveKey, pStorageState->replicatorKey(), context.Timestamp, driveCache, replicatorCache, downloadChannelCache);
		if (pDrive)
			context.Notifications.push_back(std::make_unique<model::DataModificationCancelServiceNotification<1>>(std::move(pDrive), notification.DataModificationId));
    }));
}}
