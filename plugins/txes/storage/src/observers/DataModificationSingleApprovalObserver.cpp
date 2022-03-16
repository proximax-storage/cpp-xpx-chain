/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"

namespace catapult { namespace observers {

	DEFINE_OBSERVER(DataModificationSingleApproval, model::DataModificationSingleApprovalNotification<1>, [](const model::DataModificationSingleApprovalNotification<1>& notification, ObserverContext& context) {
	  	if (NotifyMode::Rollback == context.Mode)
			CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (DataModificationSingleApproval)");

	  	auto& replicatorCache = context.Cache.sub<cache::ReplicatorCache>();
	  	auto replicatorIter = replicatorCache.find(notification.PublicKey);
	  	auto& replicatorEntry = replicatorIter.get();

	  	auto& info = replicatorEntry.drives().at(notification.DriveKey);
	  	info.LastApprovedDataModificationId = notification.DataModificationId;
	  	info.DataModificationIdIsValid = true;
		info.InitialDownloadWorkMegabytes = 0;

		auto& driveCache = context.Cache.sub<cache::BcDriveCache>();
	  	auto driveIter = driveCache.find(notification.DriveKey);
	  	auto& driveEntry = driveIter.get();

		driveEntry.confirmedUsedSizes()[notification.PublicKey] = driveEntry.usedSizeBytes();
		driveEntry.confirmedStorageInfos()[notification.PublicKey].m_confirmedStorageSince = context.Timestamp;
	});
}}
