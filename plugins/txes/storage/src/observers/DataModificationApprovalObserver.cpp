/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"

namespace catapult { namespace observers {

	DEFINE_OBSERVER(DataModificationApproval, model::DataModificationApprovalNotification<1>, [](const model::DataModificationApprovalNotification<1>& notification, ObserverContext& context) {
	  	if (NotifyMode::Rollback == context.Mode)
			CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (DataModificationApproval)");

		auto& driveCache = context.Cache.sub<cache::BcDriveCache>();
	  	auto driveIter = driveCache.find(notification.DriveKey);
	  	auto& driveEntry = driveIter.get();

		auto& completedDataModifications = driveEntry.completedDataModifications();
		auto& activeDataModifications = driveEntry.activeDataModifications();

		completedDataModifications.emplace_back(*activeDataModifications.begin(), state::DataModificationApprovalState::Approved, notification.ModificationStatus);
		activeDataModifications.erase(activeDataModifications.begin());

		auto& replicatorCache = context.Cache.sub<cache::ReplicatorCache>();
		for (const auto& replicator : driveEntry.replicators()) {
			auto replicatorIter = replicatorCache.find(replicator);
			auto& replicatorEntry = replicatorIter.get();
			auto& driveInfo = replicatorEntry.drives().at(notification.DriveKey);
			driveInfo.LastCompletedCumulativeDownloadWorkBytes += utils::FileSize::FromMegabytes(completedDataModifications.back().ActualUploadSizeMegabytes).bytes();
		}

		for (auto& [key, info]: driveEntry.confirmedStorageInfos()) {
			if (info.ConfirmedStorageSince) {
				info.TimeInConfirmedStorage = info.TimeInConfirmedStorage + context.Timestamp - *info.ConfirmedStorageSince;
			}
			info.ConfirmedStorageSince.reset();
		}

		for(int i = 0; i < notification.JudgingKeysCount + notification.OverlappingKeysCount; i++) {
			const auto& key = notification.PublicKeysPtr[i];
			driveEntry.confirmedStorageInfos()[key].ConfirmedStorageSince = context.Timestamp;
		}

		const auto totalJudgingKeysCount = notification.JudgingKeysCount + notification.OverlappingKeysCount;
		for (auto i = 0u; i < totalJudgingKeysCount; ++i)
			driveEntry.confirmedUsedSizes()[notification.PublicKeysPtr[i]] = notification.UsedDriveSize;

		driveEntry.setRootHash(notification.FileStructureCdi);
		driveEntry.setLastModificationId(completedDataModifications.back().Id);
		driveEntry.setUsedSizeBytes(notification.UsedDriveSize);
		driveEntry.setMetaFilesSizeBytes(notification.MetaFilesSize);

		driveEntry.verification().reset();
	});
}}
