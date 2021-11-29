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

//		auto replicatorIter = replicatorCacheView->find(replicatorKey);
//		auto pReplicatorEntry = replicatorIter.tryGet();
//		if (!pReplicatorEntry)
//			return replicatorData;
//
//		for (const auto& drivePair : pReplicatorEntry->drives()) {
//			auto driveIter = driveCacheView->find(drivePair.first);
//			const auto& driveEntry = driveIter.get();
//
//			DriveState driveState{
//				driveEntry.key(),
//				driveEntry.size(),
//				driveEntry.usedSize(),
//				driveEntry.replicators()
//			};
//			auto* pDriveState = &driveState;
//			replicatorData.DrivesStates.emplace_back(driveState);
//
//			const auto& activeDataModifications = driveEntry.activeDataModifications();
//			if (!activeDataModifications.empty()) {
//				pDriveState->LastDataModification = activeDataModifications.back();
//				continue;
//			}
//
//			const auto& completedDataModifications = driveEntry.completedDataModifications();
//			const auto& lastApprovedDataModification = std::find_if(
//					completedDataModifications.rbegin(),
//					completedDataModifications.rend(),
//					[](const state::CompletedDataModification& dataModification) {return dataModification.State == state::DataModificationState::Succeeded;});
//
//			if (lastApprovedDataModification == completedDataModifications.rend())
//				continue;
//
//			if (lastApprovedDataModification->Id != drivePair.second.LastApprovedDataModificationId)
//				pDriveState->LastDataModification = *lastApprovedDataModification;
//		}

		return replicatorData;
	}

	const utils::KeySet& CachedStorageState::getDrivesReplicatorsList(const Key& driveKey, cache::CatapultCache& cache) const {
		auto driveCacheView = cache.sub<cache::BcDriveCache>().createView(cache.height());
		auto driveIter = driveCacheView->find(driveKey);
		auto driveEntry = driveIter.tryGet();
		if (driveEntry)
			return driveEntry->replicators();

		return utils::KeySet{};
	}

	const BcDriveEntry* CachedStorageState::getDrive(const Key& driveKey, cache::CatapultCache& cache) const {
		auto driveCacheView = cache.sub<cache::BcDriveCache>().createView(cache.height());
		auto driveIter = driveCacheView->find(driveKey);
		return driveIter.tryGet();
	}

	const DriveInfo* CachedStorageState::getReplicatorDriveInfo(const Key& replicatorKey, const Key& driveKey, cache::CatapultCache& cache) const {
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
}}
