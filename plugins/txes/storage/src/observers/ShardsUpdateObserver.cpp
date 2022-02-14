/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <random>
#include "Observers.h"

namespace catapult { namespace observers {

	DEFINE_OBSERVER(ShardsUpdate, model::ShardsUpdateNotification<1>, ([](const model::ShardsUpdateNotification<1>& notification, ObserverContext& context) {
	  	if (NotifyMode::Rollback == context.Mode)
			CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (ShardsUpdate)");

	  	auto& driveCache = context.Cache.sub<cache::BcDriveCache>();
	  	auto driveIter = driveCache.find(notification.DriveKey);
	  	auto& driveEntry = driveIter.get();
	  	const auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::StorageConfiguration>();

		const auto& replicators = driveEntry.replicators();
	  	auto& shardsMap = driveEntry.dataModificationShards();
		std::set<Key> shardKeys;
		for (const auto& pair : shardsMap)
			shardKeys.insert(pair.first);

	  	std::set<Key> removedReplicators;
	  	std::set_difference(shardKeys.begin(), shardKeys.end(), replicators.begin(), replicators.end(),
				std::inserter(removedReplicators, removedReplicators.begin()));

	  	std::seed_seq seed(notification.Seed.begin(), notification.Seed.end());
	  	std::mt19937 rng(seed);

		// Removing respective entries in shardsMap
		for (const auto& key : removedReplicators)
			shardsMap.erase(key);

	  	// Replacing keys in other replicators' shards
		const auto shardSize = std::min<uint8_t>(pluginConfig.ShardSize, replicators.size() - 1);
	  	for (auto& pair : shardsMap) {
			auto& shardsPair = pair.second;
			for (const auto& key : removedReplicators)
				if (shardsPair.first.count(key)) {
					shardsPair.first.erase(key);
					shardsPair.second.insert(key);
				}
			const auto shardSizeDifference = shardSize - shardsPair.first.size();
			if (shardSizeDifference > 0) {
				auto& target = shardsPair.first;
				// Filtering out replicators that already belong to the shard
				std::set<Key> replicatorsSampleSource;
				std::set_difference(replicators.begin(), replicators.end(), target.begin(), target.end(),
						std::inserter(replicatorsSampleSource, replicatorsSampleSource.begin()));
				replicatorsSampleSource.erase(pair.first); // Replicator cannot be a member of his own shard
				std::sample(replicatorsSampleSource.begin(), replicatorsSampleSource.end(),
						std::inserter(target, target.end()), shardSizeDifference, rng);
			}
		}

		// Updating download shards of the drive
	  	auto& downloadCache = context.Cache.sub<cache::DownloadChannelCache>();
		if (replicators.size() <= pluginConfig.ShardSize) {
			// If drive has no more than ShardSize replicators, then each one of them
			// must be assigned to every download shard.
			for (auto& pair : driveEntry.downloadShards()) {
				auto downloadIter = downloadCache.find(pair.first);
				auto& downloadEntry = downloadIter.get();
				auto& cumulativePayments = downloadEntry.cumulativePayments();
				for (const auto& key : replicators)
					if (!cumulativePayments.count(key))
						cumulativePayments.emplace(key, Amount(0));
				// Offboarded replicators' cumulative payments remain in cumulativePayments
				downloadEntry.shardReplicators() = replicators;
				pair.second = replicators;
			}
		} else {
			std::vector<Key> sampleSource(replicators.begin(), replicators.end());
			for (auto& pair : driveEntry.downloadShards()) {
				auto downloadIter = downloadCache.find(pair.first);
				auto& downloadEntry = downloadIter.get();
				auto& cumulativePayments = downloadEntry.cumulativePayments();
				const Key downloadChannelKey = Key(pair.first.array());
				const auto comparator = [&downloadChannelKey](const Key& a, const Key& b) { return (a ^ downloadChannelKey) < (b ^ downloadChannelKey); };
				std::sort(sampleSource.begin(), sampleSource.end(), comparator);
				auto keyIter = sampleSource.begin();
				for (auto i = 0u; i < pluginConfig.ShardSize; ++i, ++keyIter)
					if (!cumulativePayments.count(*keyIter))
						cumulativePayments.emplace(*keyIter, Amount(0));
				downloadEntry.shardReplicators() = std::set<Key>(sampleSource.begin(), keyIter);	// keyIter now points to the element past the (ShardSize)th
				pair.second = std::set<Key>(sampleSource.begin(), keyIter);
			}
		}
	}));
}}
