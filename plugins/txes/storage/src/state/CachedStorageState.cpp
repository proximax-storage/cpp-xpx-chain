/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "CachedStorageState.h"
#include "catapult/cache/CatapultCache.h"
#include "src/cache/ReplicatorCache.h"
#include "src/cache/BcDriveCache.h"
#include "src/cache/DownloadChannelCache.h"

namespace catapult { namespace state {

	bool CachedStorageState::isReplicatorRegistered(const Key& key) {
		const auto& keys = m_pKeyCollector->keys();
		return (keys.find(key) != keys.end());
	}

	ReplicatorData CachedStorageState::getReplicatorData(const Key& replicatorKey, cache::CatapultCache& cache) {
		ReplicatorData replicatorData;

		auto replicatorCacheView = cache.sub<cache::ReplicatorCache>().createView(cache.height());
		auto driveCacheView = cache.sub<cache::BcDriveCache>().createView(cache.height());
		auto downloadChannelCacheView = cache.sub<cache::DownloadChannelCache>().createView(cache.height());

		auto replicatorIter = replicatorCacheView->find(replicatorKey);
		auto pReplicatorEntry = replicatorIter.tryGet();
		if (!pReplicatorEntry)
			return replicatorData;

		for (const auto& driveKey : pReplicatorEntry->drives()) {
			auto driveIter = driveCacheView->find(driveKey);
			const auto& driveEntry = driveIter.get();
			replicatorData.Drives.emplace_back(driveKey, driveEntry.size());

			for (const auto& dataModification : driveEntry.activeDataModifications())
				replicatorData.DriveModifications[driveKey].emplace_back(dataModification.Id, dataModification.DownloadDataCdi);

			for (const auto& downloadChannelId : driveEntry.activeDownloads()) {
				auto downloadChannelIter = downloadChannelCacheView->find(downloadChannelId);
				const auto& downloadChannelEntry = downloadChannelIter.get();
				replicatorData.Consumers[downloadChannelEntry.consumer()][driveKey] += downloadChannelEntry.storageUnits().unwrap();
			}
		}

		return replicatorData;
	}
}}
