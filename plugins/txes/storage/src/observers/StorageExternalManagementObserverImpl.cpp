/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "StorageExternalManagementObserverImpl.h"
#include "src/cache/BcDriveCache.h"
#include "src/cache/ReplicatorCache.h"

void catapult::observers::StorageExternalManagementObserverImpl::updateStorageState(
		const ObserverContext& context,
		const Key& driveKey,
		const Hash256& storageHash,
		const Hash256& modificationId,
		uint64_t usedSize,
		uint64_t metaFilesSize) {
	auto& driveCache = context.Cache.sub<cache::BcDriveCache>();
	auto driveIter = driveCache.find(driveKey);
	auto& driveEntry = driveIter.get();

	driveEntry.setOwnerManagement(state::OwnerManagement::PERMANENTLY_FORBIDDEN);

	for (auto& [key, info]: driveEntry.confirmedStorageInfos()) {
		if (info.ConfirmedStorageSince) {
			info.TimeInConfirmedStorage = info.TimeInConfirmedStorage + context.Timestamp - *info.ConfirmedStorageSince;
		}
		info.ConfirmedStorageSince.reset();
	}

	driveEntry.setRootHash(storageHash);
	driveEntry.setLastModificationId(modificationId);
	driveEntry.setUsedSizeBytes(usedSize);
	driveEntry.setMetaFilesSizeBytes(metaFilesSize);

	driveEntry.verification().reset();
}
void catapult::observers::StorageExternalManagementObserverImpl::addToConfirmedStorage(
		const ObserverContext& context,
		const Key& driveKey,
		const std::set<Key>& replicators) {
	auto& driveCache = context.Cache.sub<cache::BcDriveCache>();
	auto driveIter = driveCache.find(driveKey);
	auto& driveEntry = driveIter.get();

	for(const auto& key: replicators) {
		driveEntry.confirmedStorageInfos()[key].ConfirmedStorageSince = context.Timestamp;
		// TODO Maybe it is not needed
		driveEntry.confirmedUsedSizes()[key] = driveEntry.usedSizeBytes();
	}
}
void catapult::observers::StorageExternalManagementObserverImpl::allowOwnerManagement(
		const catapult::observers::ObserverContext& context,
		const catapult::Key& driveKey) {
	auto& driveCache = context.Cache.sub<cache::BcDriveCache>();
	auto driveIter = driveCache.find(driveKey);
	auto& driveEntry = driveIter.get();
	driveEntry.ownerManagement() == state::OwnerManagement::ALLOWED;
}
