/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"

namespace catapult { namespace observers {

	using Notification = model::DataModificationApprovalNotification<1>;

	DECLARE_OBSERVER(DataModificationApproval, Notification)(const std::shared_ptr<state::StorageState>& pStorageState) {
		return MAKE_OBSERVER(DataModificationApproval, Notification, ([pStorageState](const Notification& notification, ObserverContext& context) {
			if (NotifyMode::Rollback == context.Mode)
				CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (DataModificationApproval)");

			auto& driveCache = context.Cache.template sub<cache::BcDriveCache>();
			auto driveIter = driveCache.find(notification.DriveKey);
			auto& driveEntry = driveIter.get();

			auto& completedDataModifications = driveEntry.completedDataModifications();
			auto& activeDataModifications = driveEntry.activeDataModifications();

			completedDataModifications.emplace_back(*activeDataModifications.begin(), state::DataModificationApprovalState::Approved, notification.ModificationStatus);
			activeDataModifications.erase(activeDataModifications.begin());

			auto& replicatorCache = context.Cache.template sub<cache::ReplicatorCache>();
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

			const auto totalJudgingKeysCount = notification.JudgingKeysCount + notification.OverlappingKeysCount;
			for(int i = 0; i < totalJudgingKeysCount; i++) {
				const auto& key = notification.PublicKeysPtr[i];
				driveEntry.confirmedStorageInfos()[key].ConfirmedStorageSince = context.Timestamp;
			}

			for (auto i = 0u; i < totalJudgingKeysCount; ++i)
				driveEntry.confirmedUsedSizes()[notification.PublicKeysPtr[i]] = notification.UsedDriveSize;

			driveEntry.setRootHash(notification.FileStructureCdi);
			driveEntry.setLastModificationId(completedDataModifications.back().Id);
			driveEntry.setUsedSizeBytes(notification.UsedDriveSize);
			driveEntry.setMetaFilesSizeBytes(notification.MetaFilesSize);

			driveEntry.verification().reset();

			const auto& driveReplicators = driveEntry.replicators();
			if (driveReplicators.find(pStorageState->replicatorKey()) == driveReplicators.end())
				return;

			std::vector<Key> replicators(notification.PublicKeysPtr, notification.PublicKeysPtr + totalJudgingKeysCount); // TODO: validate
			auto& downloadChannelCache = context.Cache.template sub<cache::DownloadChannelCache>();
			auto pDrive = utils::GetDrive(notification.DriveKey, pStorageState->replicatorKey(), context.Timestamp, driveCache, replicatorCache, downloadChannelCache);
			auto pNotification = std::make_unique<model::DataModificationApprovalServiceNotification<1>>(std::move(pDrive), notification.DataModificationId, notification.FileStructureCdi, std::move(replicators));
			context.Notifications.push_back(std::move(pNotification));
		}))
	}
}}
