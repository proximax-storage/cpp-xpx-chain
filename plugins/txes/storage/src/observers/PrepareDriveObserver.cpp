/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"

namespace catapult { namespace observers {

	using Notification = model::PrepareDriveNotification<1>;
	using DrivePriority = std::pair<Key, double>;

	DECLARE_OBSERVER(PrepareDrive, Notification)(const std::shared_ptr<cache::ReplicatorKeyCollector>& pKeyCollector, const std::unique_ptr<std::priority_queue<DrivePriority>>& pDriveQueue) {
		return MAKE_OBSERVER(PrepareDrive, Notification, ([pKeyCollector, &pDriveQueue](const Notification& notification, const ObserverContext& context) {
			if (NotifyMode::Rollback == context.Mode)
				CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (PrepareDrive)");

			auto& driveCache = context.Cache.sub<cache::BcDriveCache>();
			state::BcDriveEntry driveEntry(notification.DriveKey);
			driveEntry.setOwner(notification.Owner);
			driveEntry.setSize(notification.DriveSize);
			driveEntry.setReplicatorCount(notification.ReplicatorCount);

		  	// Filter out replicators that are ready to be assigned to the drive,
			// i.e. which have at least (notification.DriveSize) of storage units
			// and at least (2 * notification.DriveSize) of streaming units:
			const auto& storageMosaicId = context.Config.Immutable.StorageMosaicId;
		  	const auto& streamingMosaicId = context.Config.Immutable.StreamingMosaicId;
		  	auto& accountStateCache = context.Cache.sub<cache::AccountStateCache>();
		  	const auto comparator = [&notification](const Key& a, const Key& b){ return (a ^ notification.DriveKey) < (b ^ notification.DriveKey); };
		  	std::set<Key, decltype(comparator)> acceptableReplicators(comparator);
			for (const auto& replicatorKey : pKeyCollector->keys()) {
				auto replicatorStateIter = accountStateCache.find(replicatorKey);
				auto& replicatorState = replicatorStateIter.get();
				const bool hasEnoughMosaics = replicatorState.Balances.get(storageMosaicId).unwrap() >= notification.DriveSize &&
											  replicatorState.Balances.get(streamingMosaicId).unwrap() >= 2 * notification.DriveSize;
				if (hasEnoughMosaics)
					acceptableReplicators.insert(replicatorKey);	// Inserted keys are ordered by their
																	// XOR distance to the drive key.
			}

		  	// Pick the first (notification.ReplicatorCount) replicators from acceptableReplicators
			// and assign them to the drive. If (acceptableReplicators.size() < notification.ReplicatorCount),
			// assign all that are in acceptableReplicators:
			auto driveStateIter = accountStateCache.find(notification.DriveKey);
		  	auto& driveState = driveStateIter.get();
		  	auto& replicatorCache = context.Cache.sub<cache::ReplicatorCache>();
		  	for (const auto& replicatorKey : acceptableReplicators) {
				// Updating drives() and replicators()
				auto replicatorIter = replicatorCache.find(replicatorKey);
				auto& replicatorEntry = replicatorIter.get();
				replicatorEntry.drives().emplace(notification.DriveKey, state::DriveInfo{ Hash256(), false, 0, 0 });
				driveEntry.replicators().emplace(replicatorKey);

				// Making mosaic transfers
				auto replicatorStateIter = accountStateCache.find(replicatorKey);
				auto& replicatorState = replicatorStateIter.get();
				const auto storageDepositAmount = Amount(notification.DriveSize);
				const auto streamingDepositAmount = Amount(2 * notification.DriveSize);
				replicatorState.Balances.debit(storageMosaicId, storageDepositAmount);
				replicatorState.Balances.debit(streamingMosaicId, streamingDepositAmount);
				driveState.Balances.credit(storageMosaicId, storageDepositAmount);
				driveState.Balances.credit(streamingMosaicId, streamingDepositAmount);

				if (driveEntry.replicators().size() >= notification.ReplicatorCount)
					break;
		  	}

			// If the actual number of assigned replicators is less than ordered,
			// put the drive in the queue:
		  	if (driveEntry.replicators().size() < notification.ReplicatorCount) {
				const auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::StorageConfiguration>();
				const auto drivePriority = utils::CalculateDrivePriority(driveEntry, pluginConfig.MinReplicatorCount);
				pDriveQueue->emplace(driveEntry.key(), drivePriority);
			}

			driveCache.insert(driveEntry);
		}))
	}
}}
