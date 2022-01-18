/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"

namespace catapult { namespace observers {

	using Notification = model::ReplicatorOnboardingNotification<1>;
	using DrivePriority = std::pair<Key, double>;
	using DriveQueue = std::priority_queue<DrivePriority, std::vector<DrivePriority>, utils::DriveQueueComparator>;

	DECLARE_OBSERVER(ReplicatorOnboarding, Notification)(const std::unique_ptr<DriveQueue>& pDriveQueue) {
		return MAKE_OBSERVER(ReplicatorOnboarding, Notification, ([&pDriveQueue](const Notification& notification, const ObserverContext& context) {
			if (NotifyMode::Rollback == context.Mode)
				CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (ReplicatorOnboarding)");

			auto& replicatorCache = context.Cache.sub<cache::ReplicatorCache>();
			state::ReplicatorEntry replicatorEntry(notification.PublicKey);
			replicatorEntry.setCapacity(notification.Capacity);

		  	auto& driveCache = context.Cache.sub<cache::BcDriveCache>();
		  	auto& downloadChannelCache = context.Cache.sub<cache::DownloadChannelCache>();
		  	auto& accountStateCache = context.Cache.sub<cache::AccountStateCache>();
		  	auto replicatorStateIter = accountStateCache.find(notification.PublicKey);
		  	auto& replicatorState = replicatorStateIter.get();
		  	const auto& storageMosaicId = context.Config.Immutable.StorageMosaicId;
		  	const auto& streamingMosaicId = context.Config.Immutable.StreamingMosaicId;
		  	const auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::StorageConfiguration>();

			// Assign queued drives to the replicator, as long as there is enough capacity,
			// and update download shards:
		  	DriveQueue originalQueue = *pDriveQueue;
	  		DriveQueue newQueue;
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

					// Updating download shards of the drive
					if (driveEntry.replicators().size() <= pluginConfig.ShardSize) {
						// If drive has no more than ShardSize replicators, then each one of them
						// (except the onboarding replicator) is currently assigned to every download shard.
						// Just add the onboarding replicator to every shard.
						for (auto& pair : driveEntry.downloadShards()) {
							pair.second.insert(notification.PublicKey);
							auto downloadChannelIter = downloadChannelCache.find(pair.first);
							auto& downloadChannelEntry = downloadChannelIter.get();
							downloadChannelEntry.cumulativePayments().emplace(notification.PublicKey, Amount(0));
						}
					} else {
						for (auto& pair : driveEntry.downloadShards()) {
							// For every download shard, the new replicator key will either be
							// - close enough to the download channel id (XOR-wise), replacing the most distant key of that shard, or
							// - not close enough, leaving the download shard unchanged
							const Key downloadChannelKey = Key(pair.first.array());
							auto& shardKeys = pair.second;
							auto mostDistantKeyIter = shardKeys.begin();
							auto greatestDistance = *mostDistantKeyIter ^ downloadChannelKey;
							for (auto replicatorKeyIter = ++shardKeys.begin(); replicatorKeyIter != shardKeys.end(); ++replicatorKeyIter) {
								const auto distance = *replicatorKeyIter ^ downloadChannelKey;
								if (distance > greatestDistance) {
									greatestDistance = distance;
									mostDistantKeyIter = replicatorKeyIter;
								}
							}
							if ((notification.PublicKey ^ downloadChannelKey) < greatestDistance) {
								shardKeys.erase(mostDistantKeyIter);
								shardKeys.insert(notification.PublicKey);
								auto downloadChannelIter = downloadChannelCache.find(pair.first);
								auto& downloadChannelEntry = downloadChannelIter.get();
								downloadChannelEntry.cumulativePayments().emplace(notification.PublicKey, Amount(0));
								// Cumulative payments of the removed replicator are kept in download channel entry
							}
						}
					}

					// Making mosaic transfers
					auto driveStateIter = accountStateCache.find(driveKey);
					auto& driveState = driveStateIter.get();
					const auto storageDepositAmount = Amount(driveSize);
					const auto streamingDepositAmount = Amount(2 * driveSize);
					replicatorState.Balances.debit(storageMosaicId, storageDepositAmount);
					replicatorState.Balances.debit(streamingMosaicId, streamingDepositAmount);
					driveState.Balances.credit(storageMosaicId, storageDepositAmount);
					driveState.Balances.credit(streamingMosaicId, streamingDepositAmount);

					// Keeping updated DrivePriority in newQueue if the drive still requires any replicators
					if (driveEntry.replicators().size() < driveEntry.replicatorCount()) {
						const auto newPriority = utils::CalculateDrivePriority(driveEntry, pluginConfig.MinReplicatorCount);
						newQueue.emplace(driveKey, newPriority);
					}

					// Updating remaining capacity
					remainingCapacity -= driveSize;
				} else {
					newQueue.push(drivePriorityPair);
				}
			}
			*pDriveQueue = std::move(newQueue);

			replicatorCache.insert(replicatorEntry);
		}))
	}
}}
