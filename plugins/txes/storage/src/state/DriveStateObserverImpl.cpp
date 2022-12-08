/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "DriveStateObserverImpl.h"
#include "src/cache/BcDriveCache.h"
#include <catapult/cache/ReadOnlyCatapultCache.h>

namespace catapult::state {

uint16_t catapult::state::DriveStateBrowserImpl::getOrderedReplicatorsCount(
		const catapult::cache::ReadOnlyCatapultCache& cache,
		const catapult::Key& driveKey) {
	const auto& driveCache = cache.template sub<cache::BcDriveCache>();
	auto driveIter = driveCache.find(driveKey);
	const auto& driveEntry = driveIter.get();
	return driveEntry.replicatorCount();
}
Key DriveStateBrowserImpl::getDriveOwner(const cache::ReadOnlyCatapultCache& cache, const Key& driveKey) const {
	const auto& driveCache = cache.template sub<cache::BcDriveCache>();
	auto driveIter = driveCache.find(driveKey);
	const auto& driveEntry = driveIter.get();
	return driveEntry.owner();
}

}