/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"

namespace catapult { namespace observers {

	using Notification = model::ReplicatorOnboardingNotification<1>;
	using DrivePriority = std::pair<Key, double>;

	DECLARE_OBSERVER(ReplicatorOnboarding, Notification)(const std::unique_ptr<std::priority_queue<DrivePriority>>& pDriveQueue) {
		return MAKE_OBSERVER(ReplicatorOnboarding, Notification, ([&pDriveQueue](const Notification& notification, const ObserverContext& context) {
			if (NotifyMode::Rollback == context.Mode)
				CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (ReplicatorOnboarding)");

			auto& replicatorCache = context.Cache.sub<cache::ReplicatorCache>();
			state::ReplicatorEntry replicatorEntry(notification.PublicKey);
			replicatorEntry.setCapacity(notification.Capacity);

		  	auto& driveCache = context.Cache.sub<cache::BcDriveCache>();
		  	auto& accountStateCache = context.Cache.sub<cache::AccountStateCache>();
		  	auto replicatorStateIter = accountStateCache.find(notification.PublicKey);
		  	auto& replicatorState = replicatorStateIter.get();
		  	const auto& storageMosaicId = context.Config.Immutable.StorageMosaicId;
		  	const auto& streamingMosaicId = context.Config.Immutable.StreamingMosaicId;

			// Assign queued drives to the replicator, as long as there is enough capacity:
		  	std::priority_queue<DrivePriority> originalQueue = *pDriveQueue.get();
			std::priority_queue<DrivePriority> newQueue;
			auto remainingCapacity = notification.Capacity.unwrap();
			while (!originalQueue.empty()) {
				const auto drivePriorityPair = originalQueue.top();
				const auto& driveKey = drivePriorityPair.first;
				originalQueue.pop();

				auto driveIter = driveCache.find(driveKey);
				auto& driveEntry = driveIter.get();
				const auto& driveSize = driveEntry.size();
				if (driveSize <= remainingCapacity) {
					// Updating drives() and replicators()
					replicatorEntry.drives().emplace(driveKey, state::DriveInfo{ Hash256(), false, 0 });
					driveEntry.replicators().emplace(notification.PublicKey);

					// Making mosaic transfers
					auto driveStateIter = accountStateCache.find(driveKey);
					auto& driveState = driveStateIter.get();
					const auto storageDepositAmount = Amount(driveSize);
					const auto streamingDepositAmount = Amount(2 * driveSize);
					replicatorState.Balances.debit(storageMosaicId, storageDepositAmount);
					replicatorState.Balances.debit(streamingMosaicId, streamingDepositAmount);
					driveState.Balances.credit(storageMosaicId, storageDepositAmount);
					driveState.Balances.credit(streamingMosaicId, streamingDepositAmount);

					// Pushing back updated DrivePriority to originalQueue if the drive still requires any replicators
					if (driveEntry.replicators().size() < driveEntry.replicatorCount()) {
						const auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::StorageConfiguration>();
						const auto newPriority = utils::CalculateDrivePriority(driveEntry, pluginConfig.MinReplicatorCount);
						originalQueue.emplace(driveKey, newPriority);
					}

					// Updating remaining capacity
					remainingCapacity -= driveSize;
				} else {
					newQueue.push(drivePriorityPair);
				}
			}
			*pDriveQueue.get() = std::move(newQueue);

			replicatorCache.insert(replicatorEntry);
		}))
	}
}}
