/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <random>
#include "Observers.h"
#include "Queue.h"

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
		  	auto& replicators = driveEntry.replicators();
		  	for (const auto& replicatorKey : acceptableReplicators) {
		  		// Updating the cache entries
				auto replicatorIter = replicatorCache.find(replicatorKey);
				auto& replicatorEntry = replicatorIter.get();
				replicatorEntry.drives().emplace(notification.DriveKey, state::DriveInfo{ Hash256(), false, 0, 0 });
				replicators.emplace(replicatorKey);
				CATAPULT_LOG( error ) << "added to cumulativeUploadSizesBytes " << replicatorKey;

				// Making mosaic transfers
				auto replicatorStateIter = accountStateCache.find(replicatorKey);
				auto& replicatorState = replicatorStateIter.get();
				const auto storageDepositAmount = Amount(notification.DriveSize);
				const auto streamingDepositAmount = Amount(2 * notification.DriveSize);
				replicatorState.Balances.debit(storageMosaicId, storageDepositAmount);
				replicatorState.Balances.debit(streamingMosaicId, streamingDepositAmount);
				driveState.Balances.credit(storageMosaicId, storageDepositAmount);
				driveState.Balances.credit(streamingMosaicId, streamingDepositAmount);

				if (replicators.size() >= notification.ReplicatorCount)
					break;
		  	}

			// If the actual number of assigned replicators is less than ordered,
			// put the drive in the queue:
		  	if (replicators.size() < notification.ReplicatorCount) {
				const auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::StorageConfiguration>();
				const auto drivePriority = utils::CalculateDrivePriority(driveEntry, pluginConfig.MinReplicatorCount);
				pDriveQueue->emplace(notification.DriveKey, drivePriority);
			}

			// Form initial data modification shards:
		  	const auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::StorageConfiguration>();
		  	auto& dataModificationShards = driveEntry.dataModificationShards();
		  	if (replicators.size() <= pluginConfig.ShardSize + 1) {
			  	// If the drive has no more than (ShardSize + 1) replicators, then for each one of them
			  	// all other replicators will be added to this replicator's shard.
			  	for (const auto& mainKey : replicators) {
			  		auto& mainKeyShard = dataModificationShards[mainKey];
					for (const auto& replicatorKey: replicators) {
						if (replicatorKey != mainKey) {
							mainKeyShard.m_actualShardMembers.insert({ replicatorKey, 0 });
						}
					}
				}
		  	} else {
				auto sampleSource = replicators;
				for (const auto& mainKey : replicators) {
					sampleSource.erase(mainKey);	// Replicator cannot be a member of his own shard
					std::set<Key> target;
					const auto driveKeyXorMainKey = notification.DriveKey ^ mainKey;
					std::seed_seq seed(driveKeyXorMainKey.begin(), driveKeyXorMainKey.end());
					std::sample(sampleSource.begin(), sampleSource.end(), std::inserter(target, target.end()),
								pluginConfig.ShardSize, std::mt19937(seed));
					sampleSource.insert(mainKey);	// Restoring original state of sampleSource

					auto& shard = dataModificationShards[mainKey].m_actualShardMembers;
					for (const auto& key: target) {
						shard.insert({key, 0});
					}
				}
		  	}

			driveCache.insert(driveEntry);

			// Insert the Drive into the payment Queue
		  	auto& queueCache = context.Cache.template sub<cache::QueueCache>();
		  	QueueAdapter<cache::BcDriveCache> queueAdapter(queueCache, state::DrivePaymentQueueKey, driveCache);
			queueAdapter.pushBack(driveEntry.entryKey());
		}))
	}
}}
