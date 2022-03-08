/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <random>
#include "Observers.h"

namespace catapult { namespace observers {

	DEFINE_OBSERVER(ShardsUpdate, model::ShardsUpdateNotification<1>, [](const model::ShardsUpdateNotification<1>& notification, ObserverContext& context) {
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
				if (auto it = shardsPair.m_actualShardMembers.find(key); it != shardsPair.m_actualShardMembers.end()) {
					shardsPair.m_formerShardMembers.insert(*it);
					shardsPair.m_actualShardMembers.erase(it);
				}
			const auto shardSizeDifference = shardSize - shardsPair.m_actualShardMembers.size();
			if (shardSizeDifference > 0) {
				auto initialKeys = shardsPair.getActualShardMembersKeys();
				// Filtering out replicators that already belong to the shard
				std::set<Key> replicatorsSampleSource;
				std::set_difference(replicators.begin(), replicators.end(), initialKeys.begin(), initialKeys.end(),
						std::inserter(replicatorsSampleSource, replicatorsSampleSource.begin()));
				replicatorsSampleSource.erase(pair.first); // Replicator cannot be a member of his own shard

				std::set<Key> sampledKeys;
				std::sample(replicatorsSampleSource.begin(), replicatorsSampleSource.end(),
							std::inserter(sampledKeys, sampledKeys.end()), shardSizeDifference, rng);

				for (const auto& key: sampledKeys) {
					shardsPair.m_actualShardMembers[key] = 0;
				}
			}
		}
	});
}}
