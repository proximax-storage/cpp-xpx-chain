/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"

namespace catapult { namespace observers {

	using Notification = model::StreamFinishNotification<1>;

	DECLARE_OBSERVER(StreamFinish, Notification)(const std::shared_ptr<state::StorageState>& pStorageState) {
		return MAKE_OBSERVER(StreamFinish, Notification, ([pStorageState](const Notification& notification, ObserverContext& context) {
			if (NotifyMode::Rollback == context.Mode)
				CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (StreamFinish)");

			auto& driveCache = context.Cache.sub<cache::BcDriveCache>();
			auto driveIter = driveCache.find(notification.DriveKey);
			auto& driveEntry = driveIter.get();

			auto activeDataModification = driveEntry.activeDataModifications().begin();
			activeDataModification->DownloadDataCdi = notification.StreamStructureCdi;
			activeDataModification->ActualUploadSizeMegabytes = notification.ActualUploadSize;
			activeDataModification->ReadyForApproval = true;

			const auto& replicators = driveEntry.replicators();
			if (replicators.find(pStorageState->replicatorKey()) == replicators.end())
				return;

			auto& replicatorCache = context.Cache.template sub<cache::ReplicatorCache>();
			auto& downloadChannelCache = context.Cache.template sub<cache::DownloadChannelCache>();
			auto pDrive = utils::GetDrive(notification.DriveKey, pStorageState->replicatorKey(), context.Timestamp, driveCache, replicatorCache, downloadChannelCache);
			context.Notifications.push_back(std::make_unique<model::StreamFinishServiceNotification<1>>(
				std::move(pDrive),
				notification.StreamId,
				notification.StreamStructureCdi,
				notification.ActualUploadSize));
		}))
	}
}}
