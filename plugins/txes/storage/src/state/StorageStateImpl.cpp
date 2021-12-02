/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "StorageStateImpl.h"
#include "src/cache/ReplicatorCache.h"
#include "src/cache/BcDriveCache.h"
#include "src/cache/DownloadChannelCache.h"

namespace catapult { namespace state {

	namespace {
		Drive GetDrive(const Key& driveKey, const cache::BcDriveCacheView& pDriveCacheView) {
			auto driveIter = pDriveCacheView.find(driveKey);
			auto driveEntry = driveIter.get();

			std::vector<DataModification> dataModifications;
			const auto& activeDataModifications = driveEntry.activeDataModifications();
			dataModifications.reserve(activeDataModifications.size());
			for (const auto& modification : activeDataModifications)
				dataModifications.emplace_back(DataModification{
					modification.Id,
					modification.Owner,
					modification.DownloadDataCdi,
					modification.ExpectedUploadSize});

			return Drive{
				driveEntry.key(),
				driveEntry.owner(),
				driveEntry.size(),
				driveEntry.usedSize(),
				driveEntry.replicators(),
				dataModifications
			};
		}
	}

	bool StorageStateImpl::isReplicatorRegistered(const Key& key) {
		const auto& keys = m_pKeyCollector->keys();
		return (keys.find(key) != keys.end());
	}

	Drive StorageStateImpl::getDrive(const Key& driveKey) {
		return GetDrive(driveKey, *m_pCache->sub<cache::BcDriveCache>().createView(m_pCache->height()));
	}

	uint64_t StorageStateImpl::getDownloadWork(const Key& replicatorKey, const Key& driveKey) {
		auto pReplicatorCacheView = m_pCache->sub<cache::ReplicatorCache>().createView(m_pCache->height());
		auto replicatorIter = pReplicatorCacheView->find(replicatorKey);
		auto replicatorEntry = replicatorIter.get();

		return replicatorEntry.drives().find(driveKey)->second.LastCompletedCumulativeDownloadWork;
	}

	std::vector<Drive> StorageStateImpl::getReplicatorDrives(const Key& replicatorKey) {
		auto pReplicatorCacheView = m_pCache->sub<cache::ReplicatorCache>().createView(m_pCache->height());
		auto pDriveCacheView = m_pCache->sub<cache::BcDriveCache>().createView(m_pCache->height());

		auto replicatorIter = pReplicatorCacheView->find(replicatorKey);
		auto replicatorEntry = replicatorIter.get();

		std::vector<Drive> drives;
		drives.reserve(replicatorEntry.drives().size());
		for (const auto& pair : replicatorEntry.drives())
			drives.emplace_back(GetDrive(pair.first, *pDriveCacheView));

		return drives;
	}

	std::vector<DownloadChannel> StorageStateImpl::getDownloadChannels() {
		auto downloadChannelCacheView = m_pCache->sub<cache::DownloadChannelCache>().createView(m_pCache->height());
		auto iterableView = downloadChannelCacheView->tryMakeIterableView();

		std::vector<DownloadChannel> channels;
		channels.reserve(downloadChannelCacheView->size());
		for (auto it = iterableView->begin(); it != iterableView->end(); ++it) {
			auto consumers = it->second.listOfPublicKeys();
			consumers.emplace_back(it->second.consumer());
			channels.emplace_back(DownloadChannel{it->first, it->second.downloadSize(), consumers });
		}

		return channels;
	}

}}
