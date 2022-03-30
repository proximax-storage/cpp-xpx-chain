/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"

namespace catapult { namespace observers {

	DEFINE_OBSERVER_WITH_LIQUIDITY_PROVIDER(DataModificationApprovalDownloadWork, model::DataModificationApprovalDownloadWorkNotification<1>, [&liquidityProvider](const model::DataModificationApprovalDownloadWorkNotification<1>& notification, ObserverContext& context) {
		if (NotifyMode::Rollback == context.Mode)
			CATAPULT_THROW_RUNTIME_ERROR(
					"Invalid observer mode ROLLBACK (DataModificationApprovalDownloadWork)");

		auto& replicatorCache = context.Cache.sub<cache::ReplicatorCache>();
		auto& driveCache = context.Cache.sub<cache::BcDriveCache>();
		auto driveIter = driveCache.find(notification.DriveKey);
		auto& driveEntry = driveIter.get();

		const auto& streamingMosaicId = context.Config.Immutable.StreamingMosaicId;

		auto pKey = notification.PublicKeysPtr;
		for (auto i = 0; i < notification.PublicKeysCount; ++i, ++pKey) {
			// All keys have been previously validated in DataModificationApprovalValidator.
			auto replicatorIter = replicatorCache.find(*pKey);
			auto& replicatorEntry = replicatorIter.get();

			auto& driveInfo = replicatorEntry.drives().at(notification.DriveKey);
			const auto& lastApprovedDataModificationId = driveInfo.LastApprovedDataModificationId;
			const auto& dataModificationIdIsValid = driveInfo.DataModificationIdIsValid;
			const auto& completedDataModifications = driveEntry.completedDataModifications();

			uint64_t approvableDownloadWork = 0;

			// Iterating over completed data modifications in reverse order (from newest to oldest).
			for (auto it = completedDataModifications.rbegin(); it != completedDataModifications.rend(); ++it) {
				// Exit the loop as soon as the most recent data modification approved by the replicator is
				// reached. Don't account its size. dataModificationIdIsValid prevents rare cases of premature
				// exits when the drive had no approved data modifications when the replicator joined it, but
				// current data modification id happens to match the stored lastApprovedDataModification (zero
				// hash by default).
				if (dataModificationIdIsValid && it->Id == lastApprovedDataModificationId)
					break;

				// If current data modification was approved (not cancelled), account its size.
				if (it->State == state::DataModificationState::Succeeded)
					approvableDownloadWork += it->ActualUploadSizeMegabytes;
			}

			// Making mosaic transfers.
			const auto mosaicAmount = Amount(approvableDownloadWork + driveInfo.InitialDownloadWorkMegabytes);
			liquidityProvider.debitMosaics(context, driveEntry.key(), replicatorEntry.key(), config::GetUnresolvedStreamingMosaicId(context.Config.Immutable), mosaicAmount);

			// Updating current replicator's drive info.
			driveInfo.LastApprovedDataModificationId = notification.DataModificationId;
			driveInfo.DataModificationIdIsValid = true;
			driveInfo.InitialDownloadWorkMegabytes = 0;
		}
	});
}} // namespace catapult::observers