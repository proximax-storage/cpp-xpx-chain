/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <random>
#include "Observers.h"

namespace catapult { namespace observers {

	DEFINE_OBSERVER(DownloadChannel, model::DownloadNotification<1>, [](const model::DownloadNotification<1>& notification, ObserverContext& context) {
		if (NotifyMode::Rollback == context.Mode)
			CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (DownloadChannel)");

	  	auto& downloadCache = context.Cache.sub<cache::DownloadChannelCache>();
		state::DownloadChannelEntry downloadEntry(notification.Id);
		downloadEntry.setConsumer(notification.Consumer);
	  	downloadEntry.setDrive(notification.DriveKey);
		// TODO: Buy storage units for xpx in notification.DownloadSize
		downloadEntry.setDownloadSize(notification.DownloadSize);
	  	downloadEntry.setDownloadApprovalCount(0);

		auto pKey = notification.ListOfPublicKeysPtr;
		for (auto i = 0; i < notification.ListOfPublicKeysSize; ++i, ++pKey)
			downloadEntry.listOfPublicKeys().push_back(*pKey);

		// Forming a shard:
	  	auto& driveCache = context.Cache.sub<cache::BcDriveCache>();
	  	auto driveIter = driveCache.find(notification.DriveKey);
	  	auto& driveEntry = driveIter.get();
	  	const auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::StorageConfiguration>();
		const auto& replicators = driveEntry.replicators();
	  	const auto& shardSize = std::min<uint16_t>(pluginConfig.MaxShardSize, replicators.size());
		std::set<Key> sampleReplicators;
	  	std::sample(replicators.begin(), replicators.end(), std::inserter(sampleReplicators, sampleReplicators.end()),
					shardSize, std::mt19937{std::random_device{}()});
	  	auto& cumulativePayments = downloadEntry.cumulativePayments();
		for (const auto& key : sampleReplicators)
			cumulativePayments.emplace(key, Amount(0));

		downloadCache.insert(downloadEntry);
	});
}}
