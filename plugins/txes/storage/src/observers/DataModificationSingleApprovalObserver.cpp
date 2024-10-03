/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"

namespace catapult { namespace observers {

	using Notification = model::DataModificationSingleApprovalNotification<1>;

	DECLARE_OBSERVER(DataModificationSingleApproval, Notification)(const std::shared_ptr<state::StorageState>& pStorageState) {
		return MAKE_OBSERVER(DataModificationSingleApproval, Notification, ([pStorageState](const Notification& notification, ObserverContext& context) {
			if (NotifyMode::Rollback == context.Mode)
				CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (DataModificationSingleApproval)");

			auto& replicatorCache = context.Cache.sub<cache::ReplicatorCache>();
			auto replicatorIter = replicatorCache.find(notification.PublicKey);
			auto& replicatorEntry = replicatorIter.get();

			auto& info = replicatorEntry.drives().at(notification.DriveKey);
			info.LastApprovedDataModificationId = notification.DataModificationId;
			info.InitialDownloadWorkMegabytes = 0;

			auto& driveCache = context.Cache.sub<cache::BcDriveCache>();
			auto driveIter = driveCache.find(notification.DriveKey);
			auto& driveEntry = driveIter.get();

			driveEntry.confirmedUsedSizes()[notification.PublicKey] = driveEntry.usedSizeBytes();
			driveEntry.confirmedStorageInfos()[notification.PublicKey].ConfirmedStorageSince = context.Timestamp;

			const auto& replicators = driveEntry.replicators();
			if (notification.PublicKey != pStorageState->replicatorKey() || replicators.find(pStorageState->replicatorKey()) == replicators.end())
				return;

			auto& downloadChannelCache = context.Cache.template sub<cache::DownloadChannelCache>();
			auto pDrive = utils::GetDrive(notification.DriveKey, pStorageState->replicatorKey(), context.Timestamp, driveCache, replicatorCache, downloadChannelCache);
			context.Notifications.push_back(std::make_unique<model::DataModificationSingleApprovalServiceNotification<1>>(std::move(pDrive), notification.DataModificationId, notification.PublicKey));
		}))
	}
}}
