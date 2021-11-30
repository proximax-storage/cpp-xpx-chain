/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "StorageStateImpl.h"
#include "catapult/cache/CatapultCache.h"
#include "src/cache/ReplicatorCache.h"
#include "src/cache/BcDriveCache.h"
#include "src/cache/DownloadChannelCache.h"

namespace catapult { namespace state {

	bool StorageStateImpl::isReplicatorRegistered(const Key& key) {
		const auto& keys = m_pKeyCollector->keys();
		return (keys.find(key) != keys.end());
	}

	const utils::KeySet& StorageStateImpl::getDriveReplicators(const Key& driveKey, cache::CatapultCache& cache) {
		auto driveCacheView = cache.sub<cache::BcDriveCache>().createView(cache.height());
		auto driveIter = driveCacheView->find(driveKey);
		auto driveEntry = driveIter.tryGet();
		if (driveEntry)
			return driveEntry->replicators();

		return utils::KeySet{};
	}

	const BcDriveEntry* StorageStateImpl::getDrive(const Key& driveKey, cache::CatapultCache& cache) {
		auto driveCacheView = cache.sub<cache::BcDriveCache>().createView(cache.height());
		auto driveIter = driveCacheView->find(driveKey);
		return driveIter.tryGet();
	}

	const DriveInfo* StorageStateImpl::getReplicatorDriveInfo(const Key& replicatorKey, const Key& driveKey, cache::CatapultCache& cache) {
		auto replicatorCacheView = cache.sub<cache::ReplicatorCache>().createView(cache.height());

		auto replicatorIter = replicatorCacheView->find(replicatorKey);
		auto pReplicatorEntry = replicatorIter.tryGet();
		if (!pReplicatorEntry)
			return nullptr;

		auto driveInfoIter = pReplicatorEntry->drives().find(driveKey);
		if (driveInfoIter == pReplicatorEntry->drives().end())
			return nullptr;

		return &(*driveInfoIter).second;
	}

	std::vector<const BcDriveEntry*> StorageStateImpl::getReplicatorDrives(const Key& replicatorKey, cache::CatapultCache& cache) {
		auto replicatorCacheView = cache.sub<cache::ReplicatorCache>().createView(cache.height());
		auto driveCacheView = cache.sub<cache::BcDriveCache>().createView(cache.height());

		auto replicatorIter = replicatorCacheView->find(replicatorKey);
		auto pReplicatorEntry = replicatorIter.tryGet();
		if (!pReplicatorEntry)
			return {};

		std::vector<const BcDriveEntry*> drives;
		drives.reserve(pReplicatorEntry->drives().size());

		for (const auto& drive : pReplicatorEntry->drives()) {
			auto driveIter = driveCacheView->find(drive.first);
			auto pDrive = driveIter.tryGet();
			drives.emplace_back(pDrive);
		}

		return drives;
	}

	std::vector<const DownloadChannelEntry*> StorageStateImpl::getReplicatorDownloadChannels(const Key& replicatorKey, cache::CatapultCache& cache) {
		auto downloadChannelCacheView = cache.sub<cache::DownloadChannelCache>().createView(cache.height());
		auto iterableView = downloadChannelCacheView->tryMakeIterableView();

		std::vector<const DownloadChannelEntry*> channels;
		channels.reserve(downloadChannelCacheView->size());
		for (auto it = iterableView->begin(); it != iterableView->end(); ++it) {
			channels.emplace_back(&it->second);
		}

		return channels;
	}

}}
