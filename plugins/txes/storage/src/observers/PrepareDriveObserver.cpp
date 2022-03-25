/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <random>
#include "Observers.h"
#include "src/utils/Queue.h"
#include "src/utils/AVLTree.h"

namespace catapult { namespace observers {

	using Notification = model::PrepareDriveNotification<1>;
	using DrivePriority = std::pair<Key, double>;
	using DriveQueue = std::priority_queue<DrivePriority, std::vector<DrivePriority>, utils::DriveQueueComparator>;

	DECLARE_OBSERVER(PrepareDrive, Notification)(const std::shared_ptr<cache::ReplicatorKeyCollector>& pKeyCollector, const std::shared_ptr<DriveQueue>& pDriveQueue) {
		return MAKE_OBSERVER(PrepareDrive, Notification, ([pKeyCollector, pDriveQueue](const Notification& notification, const ObserverContext& context) {
			if (NotifyMode::Rollback == context.Mode)
				CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (PrepareDrive)");

			auto& driveCache = context.Cache.sub<cache::BcDriveCache>();
			state::BcDriveEntry driveEntry(notification.DriveKey);
			driveEntry.setOwner(notification.Owner);
			driveEntry.setSize(notification.DriveSize);
			driveEntry.setReplicatorCount(notification.ReplicatorCount);
			driveEntry.setLastPayment(context.Timestamp);

			driveCache.insert(driveEntry);

		  	std::seed_seq seed(notification.DriveKey.begin(), notification.DriveKey.end());
		  	std::mt19937 rng(seed);

		  	PopulateDriveWithReplicators(notification.DriveKey, pKeyCollector, pDriveQueue, context, rng);

		  	// Insert the Drive into the payment Queue
		  	auto& queueCache = context.Cache.template sub<cache::QueueCache>();
		  	utils::QueueAdapter<cache::BcDriveCache> queueAdapter(queueCache, state::DrivePaymentQueueKey, driveCache);
		  	queueAdapter.pushBack(driveEntry.entryKey());

			// Insert the Drive into the Verification Queue
			utils::AVLTreeAdapter<Key> treeAdapter(
					queueCache,
					state::DriveVerificationsTree,
					[](const Key& key) { return key; },
					[&](const Key& key) -> state::AVLTreeNode& {
						return driveCache.find(key).get().verificationNode();
					});
			treeAdapter.insert(driveEntry.key());
		}))
	}
}}
