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

		completedDataModifications.emplace_back(*activeDataModifications.begin(), state::DataModificationState::Succeeded);
		activeDataModifications.erase(activeDataModifications.begin());

		auto& replicatorCache = context.Cache.sub<cache::ReplicatorCache>();
		for (const auto& replicator : driveEntry.replicators()) {
			auto replicatorIter = replicatorCache.find(replicator);
			auto& replicatorEntry = replicatorIter.get();

			auto& driveInfo = replicatorEntry.drives()[notification.DriveKey];
			driveInfo.LastCompletedCumulativeDownloadWork += completedDataModifications.back().ActualUploadSize;
		}

        const auto totalJudgingKeysCount = notification.JudgingKeysCount + notification.OverlappingKeysCount;
		for (auto i = 0u; i < totalJudgingKeysCount; ++i)
			driveEntry.confirmedUsedSizes().insert({notification.PublicKeysPtr[i], notification.UsedDriveSize});

		driveEntry.verifications().clear();
	});
}}
