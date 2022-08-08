/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "StorageUtils.h"
#include "src/cache/ReplicatorCache.h"
#include "src/cache/BcDriveCache.h"
#include "src/cache/DownloadChannelCache.h"
#include "src/cache/QueueCache.h"
#include "src/utils/AVLTree.h"

namespace catapult { namespace utils {

	// TODO: Clean up repeating code

	void SwapMosaics(
			const Key& sender,
			const Key& receiver,
			const std::vector<model::UnresolvedMosaic>& mosaics,
			model::NotificationSubscriber& sub,
			const config::ImmutableConfiguration& immutableCfg,
			SwapOperation operation) {
		auto currencyMosaicId = config::GetUnresolvedCurrencyMosaicId(immutableCfg);
		for (auto& mosaic : mosaics) {
			switch (operation) {
			case SwapOperation::Buy:
				sub.notify(model::BalanceDebitNotification<1>(sender, currencyMosaicId, mosaic.Amount));
				sub.notify(model::BalanceCreditNotification<1>(receiver, mosaic.MosaicId, mosaic.Amount));
				break;
			case SwapOperation::Sell:
				sub.notify(model::BalanceDebitNotification<1>(sender, mosaic.MosaicId, mosaic.Amount));
				sub.notify(model::BalanceCreditNotification<1>(receiver, currencyMosaicId, mosaic.Amount));
				break;
			default:
				CATAPULT_THROW_INVALID_ARGUMENT_1("unsupported operation", operation);
			}
		}
	}

	void SwapMosaics(
			const Key& sender,
			const Key& receiver,
			const std::vector<std::pair<UnresolvedMosaicId, UnresolvedAmount>>& mosaics,
			model::NotificationSubscriber& sub,
			const config::ImmutableConfiguration& immutableCfg,
			SwapOperation operation) {
		auto currencyMosaicId = config::GetUnresolvedCurrencyMosaicId(immutableCfg);
		for (auto& mosaic : mosaics) {
			switch (operation) {
			case SwapOperation::Buy:
				sub.notify(model::BalanceDebitNotification<1>(sender, currencyMosaicId, mosaic.second));
				sub.notify(model::BalanceCreditNotification<1>(receiver, mosaic.first, mosaic.second));
				break;
			case SwapOperation::Sell:
				sub.notify(model::BalanceDebitNotification<1>(sender, mosaic.first, mosaic.second));
				sub.notify(model::BalanceCreditNotification<1>(receiver, currencyMosaicId, mosaic.second));
				break;
			default:
				CATAPULT_THROW_INVALID_ARGUMENT_1("unsupported operation", operation);
			}
		}
	}

	void SwapMosaics(
			const Key& account,
			const std::vector<model::UnresolvedMosaic>& mosaics,
			model::NotificationSubscriber& sub,
			const config::ImmutableConfiguration& immutableCfg,
			SwapOperation operation) {
		return SwapMosaics(account, account, mosaics, sub, immutableCfg, operation);
	}

	void SwapMosaics(
			const Key& account,
			const std::vector<std::pair<UnresolvedMosaicId, UnresolvedAmount>>& mosaics,
			model::NotificationSubscriber& sub,
			const config::ImmutableConfiguration& immutableCfg,
			SwapOperation operation) {
		return SwapMosaics(account, account, mosaics, sub, immutableCfg, operation);
	}

	state::PriorityQueueEntry& getPriorityQueueEntry(cache::PriorityQueueCache::CacheDeltaType& priorityQueueCache, const Key& queueKey) {
		if (!priorityQueueCache.contains(queueKey)) {
			state::PriorityQueueEntry entry(queueKey);
			priorityQueueCache.insert(entry);
		}
		return priorityQueueCache.find(queueKey).get();
	}

	double CalculateDrivePriority(const state::BcDriveEntry& driveEntry, const uint16_t& Rmin) {
		const auto& N = driveEntry.replicatorCount();
		const auto& R = driveEntry.replicators().size() - driveEntry.offboardingReplicators().size();

		return R < Rmin ? static_cast<double>(R + 1)/Rmin : static_cast<double>(N - R)/(2*Rmin*(N - Rmin));
	}

	state::AccountState& getVoidState(const observers::ObserverContext& context) {
		auto& accountStateCache = context.Cache.sub<cache::AccountStateCache>();
		const auto zeroKey = Key();

		if (!accountStateCache.contains(zeroKey))
			accountStateCache.addAccount(zeroKey, context.Height);

		return accountStateCache.find(zeroKey).get();
	}

	void RefundDepositsToReplicators(
			const Key& driveKey,
			const std::set<Key>& replicators,
			const observers::ObserverContext& context) {
		auto& replicatorCache = context.Cache.template sub<cache::ReplicatorCache>();
		auto& accountCache = context.Cache.template sub<cache::AccountStateCache>();
		auto& driveCache = context.Cache.template sub<cache::BcDriveCache>();

		auto driveIter = driveCache.find(driveKey);
		auto& driveEntry = driveIter.get();
		auto driveStateIter = accountCache.find(driveKey);
		auto& driveState = driveStateIter.get();
		auto& voidState = getVoidState(context);

		const auto storageMosaicId = context.Config.Immutable.StorageMosaicId;
		const auto streamingMosaicId = context.Config.Immutable.StreamingMosaicId;
		const auto currencyMosaicId = context.Config.Immutable.CurrencyMosaicId;

		for (const auto& replicatorKey : replicators) {
			auto replicatorIter = replicatorCache.find(replicatorKey);
			auto& replicatorEntry = replicatorIter.get();
			auto replicatorStateIter = accountCache.find(replicatorKey);
			auto& replicatorState = replicatorStateIter.get();

			// Storage deposit equals to the drive size.
			const auto storageDepositRefundAmount = Amount(driveEntry.size());

			// Streaming Deposit Slashing equals 2 * min(u1, u2) where
			// u1 - the UsedDriveSize according to the last approved by the Replicator modification
			// u2 - the UsedDriveSize according to the last approved modification on the Drive.
			const auto& confirmedUsedSizes = driveEntry.confirmedUsedSizes();
			auto sizeIter = confirmedUsedSizes.find(replicatorKey);
            const auto streamingDepositSlashing = utils::FileSize::FromBytes(
					(confirmedUsedSizes.end() != sizeIter) ?
					2 * std::min(sizeIter->second, driveEntry.usedSizeBytes()) :
					2 * driveEntry.usedSizeBytes()
			).megabytes();

			// Streaming deposit refund = streaming deposit - streaming deposit slashing
			const auto streamingDeposit = 2 * driveEntry.size();
			if (streamingDeposit < streamingDepositSlashing)
				CATAPULT_THROW_RUNTIME_ERROR_2("streaming deposit slashing exceeds streaming deposit", streamingDeposit, streamingDepositSlashing);
			const auto streamingDepositRefundAmount = Amount(streamingDeposit - streamingDepositSlashing);

			// Making mosaic transfers
			voidState.Balances.debit(storageMosaicId, storageDepositRefundAmount, context.Height);
			driveState.Balances.debit(streamingMosaicId, streamingDepositRefundAmount, context.Height);
			replicatorState.Balances.credit(currencyMosaicId, storageDepositRefundAmount, context.Height);
			replicatorState.Balances.credit(currencyMosaicId, streamingDepositRefundAmount, context.Height);
		}
	}

	void OffboardReplicatorsFromDrive(
			const Key& driveKey,
			const std::set<Key>& offboardingReplicators,
			const observers::ObserverContext& context,
			std::mt19937& rng) {
		auto& driveCache = context.Cache.sub<cache::BcDriveCache>();
		auto driveIter = driveCache.find(driveKey);
		auto& driveEntry = driveIter.get();
		auto& replicatorCache = context.Cache.sub<cache::ReplicatorCache>();
		auto& accountStateCache = context.Cache.sub<cache::AccountStateCache>();

		for (const auto& replicatorKey : offboardingReplicators) {
			driveEntry.replicators().erase(replicatorKey);
			driveEntry.formerReplicators().insert(replicatorKey);
			driveEntry.dataModificationShards().erase(replicatorKey);
			driveEntry.confirmedUsedSizes().erase(replicatorKey);
			driveEntry.confirmedStates().erase(replicatorKey);

			auto replicatorIter = replicatorCache.find(replicatorKey);
			auto& replicatorEntry = replicatorIter.get();
			replicatorEntry.drives().erase(driveKey);
		}

		std::vector<Key> newOffboardingReplicators;
		for (const auto& replicatorKey : driveEntry.offboardingReplicators())
			if (!offboardingReplicators.count(replicatorKey))
				newOffboardingReplicators.emplace_back(replicatorKey);
		driveEntry.offboardingReplicators() = std::move(newOffboardingReplicators);

		// Replacing keys in other replicators' data modification shards
		const auto& replicators = driveEntry.replicators();
		const auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::StorageConfiguration>();
		const auto shardSize = std::min<uint8_t>(pluginConfig.ShardSize, replicators.size() - 1);
		for (auto& pair : driveEntry.dataModificationShards()) {
			auto& shardInfo = pair.second;
			for (const auto& key : offboardingReplicators)
				if (shardInfo.ActualShardMembers.count(key)) {
					shardInfo.ActualShardMembers.erase(key);
					shardInfo.FormerShardMembers.insert({key, 0});
				}
			const auto shardSizeDifference = shardSize - shardInfo.ActualShardMembers.size();
			if (shardSizeDifference > 0) {
				std::set<Key> existingKeys;
				for (const auto& [key, _]: shardInfo.ActualShardMembers) {
					existingKeys.insert(key);
				}

				// Filtering out replicators that already belong to the shard
				std::set<Key> replicatorsSampleSource;
				std::set_difference(replicators.begin(), replicators.end(), existingKeys.begin(), existingKeys.end(),
									std::inserter(replicatorsSampleSource, replicatorsSampleSource.begin()));
				replicatorsSampleSource.erase(pair.first); // Replicator cannot be a member of his own shard

				std::set<Key> target;
				std::sample(replicatorsSampleSource.begin(), replicatorsSampleSource.end(),
							std::inserter(target, target.end()), shardSizeDifference, rng);

				for (const auto& key: target) {
					shardInfo.ActualShardMembers.insert({key, 0});
				}
			}
		}

		// Updating download shards of the drive
		auto& downloadCache = context.Cache.sub<cache::DownloadChannelCache>();
		if (replicators.size() <= pluginConfig.ShardSize) {
			// If drive has no more than ShardSize replicators, then each one of them
			// must be assigned to every download shard.
			for (const auto& id : driveEntry.downloadShards()) {
				auto downloadIter = downloadCache.find(id);
				auto& downloadEntry = downloadIter.get();
				auto& cumulativePayments = downloadEntry.cumulativePayments();
				for (const auto& key : replicators)
					if (!cumulativePayments.count(key))
						cumulativePayments.emplace(key, Amount(0));
				// Offboarded replicators' cumulative payments remain in cumulativePayments
				downloadEntry.shardReplicators() = replicators;
				for (const auto& replicatorKey: downloadEntry.shardReplicators()) {
					auto& replicatorEntry = replicatorCache.find(replicatorKey).get();
					replicatorEntry.downloadChannels().insert(id);
				}
			}
		} else {
			std::vector<Key> sampleSource(replicators.begin(), replicators.end());
			for (const auto& id : driveEntry.downloadShards()) {
				auto downloadIter = downloadCache.find(id);
				auto& downloadEntry = downloadIter.get();
				auto& cumulativePayments = downloadEntry.cumulativePayments();
				const Key downloadChannelKey = Key(id.array());
				const auto comparator = [&downloadChannelKey](const Key& a, const Key& b) { return (a ^ downloadChannelKey) < (b ^ downloadChannelKey); };
				std::sort(sampleSource.begin(), sampleSource.end(), comparator);
				auto keyIter = sampleSource.begin();
				for (auto i = 0u; i < pluginConfig.ShardSize; ++i, ++keyIter)
					if (!cumulativePayments.count(*keyIter))
						cumulativePayments.emplace(*keyIter, Amount(0));
				downloadEntry.shardReplicators() = std::set<Key>(sampleSource.begin(), keyIter);	// keyIter now points to the element past the (ShardSize)th
				for (const auto& replicatorKey: downloadEntry.shardReplicators()) {
					auto& replicatorEntry = replicatorCache.find(replicatorKey).get();
					replicatorEntry.downloadChannels().insert(id);
				}
			}
		}
	}

	void UpdateShardsOnAddedReplicator(
			state::BcDriveEntry& driveEntry,
			const Key& replicatorKey,
			const observers::ObserverContext& context,
			std::mt19937& rng) {
		auto& downloadCache = context.Cache.sub<cache::DownloadChannelCache>();
		auto& replicatorCache = context.Cache.sub<cache::ReplicatorCache>();
		const auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::StorageConfiguration>();

		// Updating download shards
		if (driveEntry.replicators().size() <= pluginConfig.ShardSize) {
			// If drive has no more than ShardSize replicators, then each one of them
			// (except the onboarding replicator) is currently assigned to every download shard.
			// Just add the onboarding replicator to every shard.
			for (const auto& id : driveEntry.downloadShards()) {
				auto downloadIter = downloadCache.find(id);
				auto& downloadEntry = downloadIter.get();
				downloadEntry.shardReplicators().insert(replicatorKey);
				downloadEntry.cumulativePayments().emplace(replicatorKey, Amount(0));
				auto& replicatorEntry = replicatorCache.find(replicatorKey).get();
				replicatorEntry.downloadChannels().insert(id);
			}
		} else {
			for (const auto& id : driveEntry.downloadShards()) {
				// For every download shard, the new replicator key will either be
				// - close enough to the download channel id (XOR-wise), replacing the most distant key of that shard, or
				// - not close enough, leaving the download shard unchanged
				const Key downloadChannelKey = Key(id.array());

				auto downloadIter = downloadCache.find(id);
				auto& downloadEntry = downloadIter.get();
				auto& driveShardKeys = downloadEntry.shardReplicators();

				auto mostDistantKeyIter = driveShardKeys.begin();
				auto greatestDistance = *mostDistantKeyIter ^ downloadChannelKey;
				for (auto replicatorKeyIter = ++driveShardKeys.begin(); replicatorKeyIter != driveShardKeys.end(); ++replicatorKeyIter) {
					const auto distance = *replicatorKeyIter ^ downloadChannelKey;
					if (distance > greatestDistance) {
						greatestDistance = distance;
						mostDistantKeyIter = replicatorKeyIter;
					}
				}
				if ((replicatorKey ^ downloadChannelKey) < greatestDistance) {
					downloadEntry.shardReplicators().erase(*mostDistantKeyIter);
					downloadEntry.shardReplicators().insert(replicatorKey);

					auto& removedReplicatorEntry = replicatorCache.find(*mostDistantKeyIter).get();
					removedReplicatorEntry.downloadChannels().erase(id);

					auto& addedReplicatorEntry = replicatorCache.find(replicatorKey).get();
					addedReplicatorEntry.downloadChannels().insert(id);

					downloadEntry.cumulativePayments().emplace(replicatorKey, Amount(0));
					// Cumulative payments of the removed replicator are kept in download channel entry
				}
			}
		}

		// Updating data modification shards
		auto& shardsMap = driveEntry.dataModificationShards();
		std::set<Key> shardKeys;
		for (const auto& pair : shardsMap)
			shardKeys.insert(pair.first);

		auto replicatorsSampleSource = driveEntry.replicators();
		const auto replicatorsSize = replicatorsSampleSource.size();
		replicatorsSampleSource.erase(replicatorKey); // Replicator cannot be a member of his own shard

		if (replicatorsSize <= pluginConfig.ShardSize + 1) {
			// Adding the new replicator to all existing shards
			for (auto& pair : shardsMap) {
				auto& shardsPair = pair.second;
				shardsPair.ActualShardMembers.insert({replicatorKey, 0});
			}
			// Creating an entry for the new replicator in shardsMap
			auto& replicatorKeyShard = shardsMap[replicatorKey].ActualShardMembers;
			for (const auto& key: replicatorsSampleSource) {
				 replicatorKeyShard.insert({key, 0});
			}
		} else {
			// Selecting random shards to which the new replicator will be added
			const auto sampleSize = pluginConfig.ShardSize * replicatorsSize / (replicatorsSize - 1);
			std::set<Key> sampledShardKeys;
			std::sample(shardKeys.begin(), shardKeys.end(),
						std::inserter(sampledShardKeys, sampledShardKeys.end()), sampleSize, rng);
			// Updating selected shards
			for (auto& sampledKey : sampledShardKeys) {
				auto& shardsPair = shardsMap[sampledKey];
				if (shardsPair.ActualShardMembers.size() == pluginConfig.ShardSize) {	// TODO: Remove size check?
					const auto replacedKeyIndex = rng() % pluginConfig.ShardSize;
					auto replacedKeyIter = shardsPair.ActualShardMembers.begin();
					std::advance(replacedKeyIter, replacedKeyIndex);
					shardsPair.FormerShardMembers.insert(*replacedKeyIter);
					shardsPair.ActualShardMembers.erase(replacedKeyIter);
				}
				shardsPair.ActualShardMembers.insert({replicatorKey, 0});
			}
			// Creating an entry for the new replicator in shardsMap

			std::set<Key> target;
			std::sample(replicatorsSampleSource.begin(), replicatorsSampleSource.end(),
						std::inserter(target, target.end()), pluginConfig.ShardSize, rng);

			auto& newShardEntry = shardsMap[replicatorKey].ActualShardMembers;
			for (const auto& key: target) {
				newShardEntry.insert({key, 0});
			}
		}
	}

	void PopulateDriveWithReplicators(
			const Key& driveKey,
			const observers::ObserverContext& context,
			std::mt19937& rng) {
		auto& replicatorCache = context.Cache.sub<cache::ReplicatorCache>();
		auto& driveCache = context.Cache.sub<cache::BcDriveCache>();
		auto& priorityQueueCache = context.Cache.sub<cache::PriorityQueueCache>();
		auto& downloadCache = context.Cache.sub<cache::DownloadChannelCache>();
		auto& accountStateCache = context.Cache.sub<cache::AccountStateCache>();
		const auto& storageMosaicId = context.Config.Immutable.StorageMosaicId;
		const auto& streamingMosaicId = context.Config.Immutable.StreamingMosaicId;

		auto& driveEntry = driveCache.find(driveKey).get();
		const auto driveSize = driveEntry.size();
		const auto requiredReplicatorCount = driveEntry.replicatorCount() - driveEntry.replicators().size();

		if (requiredReplicatorCount == 0)
			return;

		// Filter out replicators that are ready to be assigned to the drive,
		// i.e. which have at least (driveSize) of storage units
		// and at least (2 * driveSize) of streaming units:

		auto keyExtractor = [=, &accountStateCache](const Key& key) {
			return std::make_pair(accountStateCache.find(key).get().Balances.get(storageMosaicId), key);
		};

		utils::AVLTreeAdapter<std::pair<Amount, Key>> treeAdapter(
				context.Cache.template sub<cache::QueueCache>(),
				state::ReplicatorsSetTree,
				keyExtractor,
				[&replicatorCache](const Key& key) -> state::AVLTreeNode {
					return replicatorCache.find(key).get().replicatorsSetNode();
				},
				[&replicatorCache](const Key& key, const state::AVLTreeNode& node) {
					replicatorCache.find(key).get().replicatorsSetNode() = node;
				});

		// Current Replicators of the Drive MUST NOT be assigned to the Drive one more time
		for (const auto& replicatorKey: driveEntry.replicators()) {
			std::pair<Amount, Key> keyToRemove = keyExtractor(replicatorKey);
			treeAdapter.remove(keyToRemove);
		}

		// Former Replicators of the Drive are banned for this Drive
		for (const auto& replicatorKey: driveEntry.formerReplicators()) {
			std::pair<Amount, Key> keyToRemove = keyExtractor(replicatorKey);
			treeAdapter.remove(keyToRemove);
		}

		auto notSuitableReplicators = treeAdapter.numberOfLess({Amount(driveSize), Key()});
		auto suitableReplicators = treeAdapter.size() - notSuitableReplicators;

		auto replicatorsToAdd = std::min(suitableReplicators, static_cast<uint32_t>(requiredReplicatorCount));

		// Preparing DriveInfo:
		const auto& completedDataModifications = driveEntry.completedDataModifications();
		const auto lastApprovedDataModificationIter = std::find_if(
				completedDataModifications.rbegin(),
				completedDataModifications.rend(),
				[](const state::CompletedDataModification& modification){
					return modification.State == state::DataModificationState::Succeeded;
				});
		const bool succeededVerifications = lastApprovedDataModificationIter != completedDataModifications.rend();
		const auto lastApprovedDataModificationId = succeededVerifications ? lastApprovedDataModificationIter->Id : Hash256();
		const auto initialDownloadWork = driveEntry.usedSizeBytes() - driveEntry.metaFilesSizeBytes();
		const state::DriveInfo driveInfo{ lastApprovedDataModificationId, initialDownloadWork, initialDownloadWork };

		// Pick the first (requiredReplicatorCount) replicators from acceptableReplicators
		// and assign them to the drive. If (acceptableReplicators.size() < requiredReplicatorCount),
		// assign all that are in acceptableReplicators:
		auto& replicators = driveEntry.replicators();
		auto driveStateIter = accountStateCache.find(driveKey);
		auto& driveState = driveStateIter.get();
		auto& voidState = getVoidState(context);
		for (int i = 0; i < replicatorsToAdd; i++) {
			uint32_t index = rng() % suitableReplicators;
			suitableReplicators--;
			auto replicatorKey = treeAdapter.extractOrderStatistics(notSuitableReplicators + index);

			// Updating the cache entries
			auto replicatorIter = replicatorCache.find(replicatorKey);
			auto& replicatorEntry = replicatorIter.get();
			replicatorEntry.drives().emplace(driveKey, driveInfo);
			replicators.emplace(replicatorKey);

			state::ConfirmedStorageInfo confirmedStorageInfo;
			if (driveEntry.completedDataModifications().empty()) {
				confirmedStorageInfo.ConfirmedStorageSince = context.Timestamp;
			}
			driveEntry.confirmedStorageInfos().insert({ replicatorKey, confirmedStorageInfo });

			// Updating drive's shards
			UpdateShardsOnAddedReplicator(driveEntry, replicatorKey, context, rng);

			// Making mosaic transfers
			auto replicatorStateIter = accountStateCache.find(replicatorKey);
			auto& replicatorState = replicatorStateIter.get();
			const auto storageDepositAmount = Amount(driveSize);
			const auto streamingDepositAmount = Amount(2 * driveSize);
			replicatorState.Balances.debit(storageMosaicId, storageDepositAmount);
			replicatorState.Balances.debit(streamingMosaicId, streamingDepositAmount);
			voidState.Balances.credit(storageMosaicId, storageDepositAmount);
			driveState.Balances.credit(streamingMosaicId, streamingDepositAmount);
		}

		for (const auto& replicatorKey: driveEntry.replicators()) {
			treeAdapter.insert(replicatorKey);
		}

		for (const auto& replicatorKey: driveEntry.formerReplicators()) {
			treeAdapter.insert(replicatorKey);
		}

		// If the actual number of assigned replicators is less than ordered,
		// put the drive in the queue:
		auto& driveQueueEntry = getPriorityQueueEntry(priorityQueueCache, state::DrivePriorityQueueKey);
		if (replicators.size() < driveEntry.replicatorCount()) {
			const auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::StorageConfiguration>();
			const auto drivePriority = utils::CalculateDrivePriority(driveEntry, pluginConfig.MinReplicatorCount);
			driveQueueEntry.set(driveKey, drivePriority);
		} else {
			driveQueueEntry.remove(driveKey);
		}
	}

	void AssignReplicatorsToQueuedDrives(
			const std::set<Key>& replicatorKeys,
			const observers::ObserverContext& context,
			std::mt19937& rng) {
		auto& replicatorCache = context.Cache.sub<cache::ReplicatorCache>();
		auto& driveCache = context.Cache.sub<cache::BcDriveCache>();
		auto& priorityQueueCache = context.Cache.sub<cache::PriorityQueueCache>();
		auto& downloadCache = context.Cache.sub<cache::DownloadChannelCache>();
		auto& accountStateCache = context.Cache.sub<cache::AccountStateCache>();
		auto& voidState = getVoidState(context);

		const auto& storageMosaicId = context.Config.Immutable.StorageMosaicId;
		const auto& streamingMosaicId = context.Config.Immutable.StreamingMosaicId;
		const auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::StorageConfiguration>();

		auto keyExtractor = [=, &accountStateCache](const Key& key) {
			return std::make_pair(accountStateCache.find(key).get().Balances.get(storageMosaicId), key);
		};

		utils::AVLTreeAdapter<std::pair<Amount, Key>> treeAdapter(
				context.Cache.template sub<cache::QueueCache>(),
						state::ReplicatorsSetTree,
						keyExtractor,
						[&replicatorCache](const Key& key) -> state::AVLTreeNode {
					return replicatorCache.find(key).get().replicatorsSetNode();
					},
					[&replicatorCache](const Key& key, const state::AVLTreeNode& node) {
					replicatorCache.find(key).get().replicatorsSetNode() = node;
				});

		// Tree Adapter does NOT contain the Replicators

		for (const auto& replicatorKey : replicatorKeys) {
			auto replicatorIter = replicatorCache.find(replicatorKey);
			auto& replicatorEntry = replicatorIter.get();
			auto replicatorStateIter = accountStateCache.find(replicatorKey);
			auto& replicatorState = replicatorStateIter.get();

			// Assign queued drives to the replicator, as long as there is enough capacity,
			// and update drive's shards:
			auto& driveQueueEntry = getPriorityQueueEntry(priorityQueueCache, state::DrivePriorityQueueKey);
			auto& originalQueue = driveQueueEntry.priorityQueue();
			std::priority_queue<state::PriorityPair> newQueue;
			const auto storageMosaicAmount = replicatorState.Balances.get(storageMosaicId);
			auto remainingCapacity = storageMosaicAmount.unwrap();
			while (!originalQueue.empty()) {
				const auto drivePriorityPair = originalQueue.top();
				const auto& driveKey = drivePriorityPair.Key;
				originalQueue.pop();

				auto driveIter = driveCache.find(driveKey);
				auto& driveEntry = driveIter.get();
				const auto& driveSize = driveEntry.size();
				bool replicatorIsBanned =
						driveEntry.formerReplicators().find(replicatorKey) != driveEntry.formerReplicators().end();
				if (driveSize <= remainingCapacity && !replicatorIsBanned) {
					// Updating drives() and replicators()
					const auto& completedDataModifications = driveEntry.completedDataModifications();
					const auto lastApprovedDataModificationIter = std::find_if(
							completedDataModifications.rbegin(),
							completedDataModifications.rend(),
							[](const state::CompletedDataModification& modification){
							  	return modification.State == state::DataModificationState::Succeeded;
							});
					const bool succeededVerifications = lastApprovedDataModificationIter != completedDataModifications.rend();
					const auto lastApprovedDataModificationId = succeededVerifications ? lastApprovedDataModificationIter->Id : Hash256();
					const auto initialDownloadWork = driveEntry.usedSizeBytes() - driveEntry.metaFilesSizeBytes();
					replicatorEntry.drives().emplace(driveKey, state::DriveInfo{
						lastApprovedDataModificationId, initialDownloadWork, initialDownloadWork
					});
					driveEntry.replicators().emplace(replicatorKey);

					state::ConfirmedStorageInfo confirmedStorageInfo;
					if (driveEntry.rootHash() == Hash256()) {
						confirmedStorageInfo.ConfirmedStorageSince = context.Timestamp;
					}
					driveEntry.confirmedStorageInfos().insert({ replicatorKey, confirmedStorageInfo });

					// Updating drive's shards
					UpdateShardsOnAddedReplicator(driveEntry, replicatorKey, context, rng);

					// Making mosaic transfers
					auto driveStateIter = accountStateCache.find(driveKey);
					auto& driveState = driveStateIter.get();
					const auto storageDepositAmount = Amount(driveSize);
					const auto streamingDepositAmount = Amount(2 * driveSize);
					replicatorState.Balances.debit(storageMosaicId, storageDepositAmount);
					replicatorState.Balances.debit(streamingMosaicId, streamingDepositAmount);
					voidState.Balances.credit(storageMosaicId, storageDepositAmount);
					driveState.Balances.credit(streamingMosaicId, streamingDepositAmount);

					// Keeping updated DrivePriority in newQueue if the drive still requires any replicators
					if (driveEntry.replicators().size() < driveEntry.replicatorCount()) {
						const auto newPriority = utils::CalculateDrivePriority(driveEntry, pluginConfig.MinReplicatorCount);
						newQueue.push( {driveKey, newPriority} );
					}

					// Updating remaining capacity
					remainingCapacity -= driveSize;
				} else {
					newQueue.push(drivePriorityPair);
				}
			}
			originalQueue = std::move(newQueue);
		}
		for (const auto& replicatorKey: replicatorKeys) {
			treeAdapter.insert(replicatorKey);
		}
	}
}}
