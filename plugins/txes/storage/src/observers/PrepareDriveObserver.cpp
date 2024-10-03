/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/utils/AVLTree.h"
#include "src/utils/Queue.h"
#include <random>

namespace catapult { namespace observers {

	using Notification = model::PrepareDriveNotification<1>;

	DECLARE_OBSERVER(PrepareDrive, Notification)(const std::shared_ptr<state::StorageState>& pStorageState) {
		return MAKE_OBSERVER(PrepareDrive, Notification, ([pStorageState](const Notification& notification, ObserverContext& context) {
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

		  	PopulateDriveWithReplicators(notification.DriveKey, context, rng);

		  	// Insert the Drive into the payment Queue
		  	auto& queueCache = context.Cache.template sub<cache::QueueCache>();
		  	utils::QueueAdapter<cache::BcDriveCache> queueAdapter(queueCache, state::DrivePaymentQueueKey, driveCache);
		  	queueAdapter.pushBack(driveEntry.entryKey());

			// Insert the Drive into the Verification Queue
			utils::AVLTreeAdapter<Key> treeAdapter(
				queueCache,
				state::DriveVerificationsTree,
				[](const Key& key) { return key; },
				[&driveCache](const Key& key) -> state::AVLTreeNode& {
					return driveCache.find(key).get().verificationNode();
				},
				[&driveCache](const Key& key, const state::AVLTreeNode& node) {
					return driveCache.find(key).get().verificationNode() = node;
				});
			treeAdapter.insert(driveEntry.key());

			const auto& replicators = driveEntry.replicators();
			if (replicators.find(pStorageState->replicatorKey()) == replicators.end())
				return;

			auto& replicatorCache = context.Cache.template sub<cache::ReplicatorCache>();
			auto& downloadChannelCache = context.Cache.template sub<cache::DownloadChannelCache>();
			auto pDrive = utils::GetDrive(notification.DriveKey, pStorageState->replicatorKey(), context.Timestamp, driveCache, replicatorCache, downloadChannelCache);
			context.Notifications.push_back(std::make_unique<model::PrepareDriveServiceNotification<1>>(std::move(pDrive)));
		}))
	}
}}
