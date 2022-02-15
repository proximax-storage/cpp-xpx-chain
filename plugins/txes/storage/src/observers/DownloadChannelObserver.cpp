/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <random>
#include "Observers.h"

namespace catapult { namespace observers {

	DEFINE_OBSERVER(DownloadChannel, model::DownloadNotification<1>, ([](const model::DownloadNotification<1>& notification, ObserverContext& context) {
		if (NotifyMode::Rollback == context.Mode)
			CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (DownloadChannel)");

	  	auto& downloadCache = context.Cache.sub<cache::DownloadChannelCache>();
		state::DownloadChannelEntry downloadEntry(notification.Id);
		downloadEntry.setConsumer(notification.Consumer);
	  	downloadEntry.setDrive(notification.DriveKey);
		// TODO: Buy storage units for xpx in notification.DownloadSize
		downloadEntry.setDownloadSize(notification.DownloadSizeMegabytes);
	  	downloadEntry.setDownloadApprovalCount(0);

		if (notification.ListOfPublicKeysSize == 0) {
			downloadEntry.listOfPublicKeys().push_back(notification.Consumer);
		} else {
			for (auto i = 0; i < notification.ListOfPublicKeysSize; ++i)
				downloadEntry.listOfPublicKeys().push_back(notification.ListOfPublicKeysPtr[i]);
		}

		// Forming a shard:
	  	auto& driveCache = context.Cache.sub<cache::BcDriveCache>();
	  	auto driveIter = driveCache.find(notification.DriveKey);
	  	auto& driveEntry = driveIter.get();
	  	const auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::StorageConfiguration>();
		const auto& replicators = driveEntry.replicators();
	  	auto& cumulativePayments = downloadEntry.cumulativePayments();
		if (replicators.size() <= pluginConfig.ShardSize) {
			// When the drive has no more than ShardSize replicators, add them all
			for (const auto& key : replicators) {
				cumulativePayments.emplace(key, Amount(0));
				driveEntry.downloadShards()[notification.Id].insert(key);
			}
		} else {
			// Otherwise, add ShardSize closest replicators in terms of XOR distance to the download channel id.
			const Key downloadChannelKey = Key(notification.Id.array());
			const auto comparator = [&downloadChannelKey](const Key& a, const Key& b) { return (a ^ downloadChannelKey) < (b ^ downloadChannelKey); };
			std::set<Key, decltype(comparator)> shardKeys(comparator);
			for (const auto& key : replicators)
				shardKeys.insert(key);
			auto keyIter = shardKeys.begin();
			for (auto i = 0u; i < pluginConfig.ShardSize; ++i) {
				cumulativePayments.emplace(*keyIter++, Amount(0));
				driveEntry.downloadShards()[notification.Id].insert(*keyIter++);
			}
		}

		downloadCache.insert(downloadEntry);
	}));
}}
