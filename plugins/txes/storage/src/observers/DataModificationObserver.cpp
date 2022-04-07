/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/utils/StorageUtils.h"

namespace catapult { namespace observers {

	using Notification = model::DataModificationNotification<1>;
	using DrivePriority = std::pair<Key, double>;
	using DriveQueue = std::priority_queue<DrivePriority, std::vector<DrivePriority>, utils::DriveQueueComparator>;

	DECLARE_OBSERVER(DataModification, Notification)(const std::shared_ptr<cache::ReplicatorKeyCollector>& pKeyCollector, const std::shared_ptr<DriveQueue>& pDriveQueue, const LiquidityProviderExchangeObserver& liquidityProvider) {
		return MAKE_OBSERVER(DataModification, Notification, ([pKeyCollector, pDriveQueue, &liquidityProvider](const Notification& notification, ObserverContext& context) {
			if (NotifyMode::Rollback == context.Mode)
				CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (DataModification)");

			auto& driveCache = context.Cache.sub<cache::BcDriveCache>();
			auto driveIter = driveCache.find(notification.DriveKey);
			auto& driveEntry = driveIter.get();

			auto& activeDataModifications = driveEntry.activeDataModifications();
			activeDataModifications.emplace_back(state::ActiveDataModification(
				notification.DataModificationId,
				notification.Owner,
				notification.DownloadDataCdi,
				notification.UploadSizeMegabytes
			));

			std::seed_seq seed(notification.DataModificationId.begin(), notification.DataModificationId.end());
			std::mt19937 rng(seed);

		  	const auto offboardingReplicators = driveEntry.offboardingReplicators();

		  	utils::RefundDepositsToReplicators(notification.DriveKey, offboardingReplicators, context, liquidityProvider);
			utils::OffboardReplicatorsFromDrive(notification.DriveKey, offboardingReplicators, context, rng);
		  	utils::PopulateDriveWithReplicators(notification.DriveKey, pKeyCollector, pDriveQueue, context, rng);
		  	utils::AssignReplicatorsToQueuedDrives(offboardingReplicators, pDriveQueue, context, rng);
		}))
	}
}}
