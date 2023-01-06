/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once

#include "catapult/cache/CatapultCache.h"
#include "catapult/state/DriveStateBrowser.h"

namespace catapult::state {

class DriveStateBrowserImpl : public DriveStateBrowser {
public:
	uint16_t getOrderedReplicatorsCount(const cache::ReadOnlyCatapultCache& cache, const Key& driveKey) const override;

	Key getDriveOwner(const cache::ReadOnlyCatapultCache& cache, const Key& driveKey) const override;

	std::set<Key> getReplicators(const cache::ReadOnlyCatapultCache& cache, const Key& driveKey) const override;

	std::set<Key> getDrives(const cache::ReadOnlyCatapultCache &cache, const Key& replicatorKey) const override;

	Hash256 getDriveState(const cache::ReadOnlyCatapultCache& cache, const Key& driveKey) const override;
};

} // namespace catapult::state