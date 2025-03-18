/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <random>
#include "Observers.h"
#include "src/utils/Queue.h"

namespace catapult { namespace observers {

	using Notification = model::DownloadNotification<1>;

	DECLARE_OBSERVER(DownloadChannel, Notification)(const std::shared_ptr<state::StorageState>& pStorageState) {
		return MAKE_OBSERVER(DownloadChannel, Notification, ([pStorageState](const Notification& notification, ObserverContext& context) {
			if (NotifyMode::Rollback == context.Mode)
				CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (DownloadChannel)");

			auto& downloadCache = context.Cache.sub<cache::DownloadChannelCache>();
			state::DownloadChannelEntry downloadEntry(notification.Id);
			downloadEntry.setConsumer(notification.Consumer);
			downloadEntry.setDrive(notification.DriveKey);
			downloadEntry.setDownloadSize(notification.DownloadSizeMegabytes);
			downloadEntry.setDownloadApprovalCountLeft(1);
			downloadEntry.setLastDownloadApprovalInitiated(context.Timestamp);

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

			driveEntry.downloadShards().insert(notification.Id);

			const auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::StorageConfiguration>();
			const auto& replicators = driveEntry.replicators();
			auto& shardReplicators = downloadEntry.shardReplicators();
			auto& cumulativePayments = downloadEntry.cumulativePayments();
			if (replicators.size() <= pluginConfig.ShardSize) {
				// When the drive has no more than ShardSize replicators, add them all
				for (const auto& key : replicators) {
					shardReplicators.insert(key);
					cumulativePayments.emplace(key, Amount(0));
				}
			} else {
				// Otherwise, add ShardSize of the closest replicators in terms of XOR distance to the download channel id.
				const Key downloadChannelKey = Key(notification.Id.array());
				std::vector<Key> sampleSource(replicators.begin(), replicators.end());
				const auto comparator = [&downloadChannelKey](const Key& a, const Key& b) { return (a ^ downloadChannelKey) < (b ^ downloadChannelKey); };
				std::sort(sampleSource.begin(), sampleSource.end(), comparator);
				auto keyIter = sampleSource.begin();
				for (auto i = 0u; i < pluginConfig.ShardSize; ++i, ++keyIter) {
					shardReplicators.insert(*keyIter);
					cumulativePayments.emplace(*keyIter, Amount(0));
				}
			}

			auto& replicatorCache = context.Cache.sub<cache::ReplicatorCache>();
			for (const auto& key: shardReplicators) {
				auto replicatorIter = replicatorCache.find(key);
				auto& replicatorEntry = replicatorIter.get();
				replicatorEntry.downloadChannels().insert(notification.Id);
			}

			downloadCache.insert(downloadEntry);

			// Insert the Drive into the payment Queue
			auto& queueCache = context.Cache.template sub<cache::QueueCache>();
			utils::QueueAdapter<cache::DownloadChannelCache> queueAdapter(queueCache, state::DownloadChannelPaymentQueueKey, downloadCache);
			queueAdapter.pushBack(downloadEntry.entryKey());

			if (shardReplicators.find(pStorageState->replicatorKey()) == shardReplicators.end())
				return;

			auto pChannel = utils::GetDownloadChannel(pStorageState->replicatorKey(), downloadEntry);
			context.Notifications.push_back(std::make_unique<model::DownloadServiceNotification<1>>(std::move(pChannel)));
		}))
	}
}}
