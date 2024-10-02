/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"

namespace catapult { namespace observers {

	using Notification = model::StreamPaymentNotification<1>;

	DECLARE_OBSERVER(StreamPayment, Notification)(const std::shared_ptr<state::StorageState>& pStorageState) {
		return MAKE_OBSERVER(StreamPayment, Notification, ([pStorageState](const Notification& notification, ObserverContext& context) {
			if (NotifyMode::Rollback == context.Mode)
				CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (StreamPayment)");

			auto& driveCache = context.Cache.sub<cache::BcDriveCache>();
			auto driveIter = driveCache.find(notification.DriveKey);
			auto& driveEntry = driveIter.get();

			auto& activeDataModifications = driveEntry.activeDataModifications();
			auto pModification = std::find_if(
					activeDataModifications.begin(),
					activeDataModifications.end(),
					[notification] (const state::ActiveDataModification& modification) -> bool {
						return notification.StreamId == modification.Id;
					});
			pModification->ExpectedUploadSizeMegabytes += notification.AdditionalUploadSize;
			pModification->ActualUploadSizeMegabytes += notification.AdditionalUploadSize;

			const auto& replicators = driveEntry.replicators();
			if (replicators.find(pStorageState->replicatorKey()) == replicators.end())
				return;

			auto& replicatorCache = context.Cache.template sub<cache::ReplicatorCache>();
			auto& downloadChannelCache = context.Cache.template sub<cache::DownloadChannelCache>();
			auto pDrive = utils::GetDrive(notification.DriveKey, pStorageState->replicatorKey(), context.Timestamp, driveCache, replicatorCache, downloadChannelCache);
			context.Notifications.push_back(std::make_unique<model::StreamPaymentServiceNotification<1>>(
				std::move(pDrive),
				notification.StreamId));
		}))
	}
}}
