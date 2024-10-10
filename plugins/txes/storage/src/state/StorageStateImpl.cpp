/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "StorageStateImpl.h"
#include "src/cache/ReplicatorCache.h"
#include "src/cache/BcDriveCache.h"
#include "src/cache/DownloadChannelCache.h"
#include "src/utils/StorageUtils.h"

namespace catapult { namespace state {

    bool StorageStateImpl::isReplicatorRegistered() {
		auto pReplicatorCacheView = m_pCache->sub<cache::ReplicatorCache>().createView(m_pCache->height());
		return pReplicatorCacheView->contains(m_replicatorKey);
    }

	std::shared_ptr<Drive> StorageStateImpl::getDrive(const Key& driveKey, const Timestamp& timestamp) {
		auto height = m_pCache->height();
		auto pDriveCacheView = m_pCache->sub<cache::BcDriveCache>().createView(height);
		auto pReplicatorCacheView = m_pCache->sub<cache::ReplicatorCache>().createView(height);
		auto pDownloadChannelCacheView = m_pCache->sub<cache::DownloadChannelCache>().createView(height);
        return utils::GetDrive(
			driveKey,
			m_replicatorKey,
			timestamp,
			*pDriveCacheView,
			*pReplicatorCacheView,
			*pDownloadChannelCacheView);
    }

    std::vector<std::shared_ptr<Drive>> StorageStateImpl::getDrives(const Timestamp& timestamp) {
		auto height = m_pCache->height();
		auto pDriveCacheView = m_pCache->sub<cache::BcDriveCache>().createView(height);
		auto pReplicatorCacheView = m_pCache->sub<cache::ReplicatorCache>().createView(height);
		auto pDownloadChannelCacheView = m_pCache->sub<cache::DownloadChannelCache>().createView(height);

        auto replicatorIter = pReplicatorCacheView->find(m_replicatorKey);
		const auto& replicatorEntry = replicatorIter.get();

        std::vector<std::shared_ptr<Drive>> drives;
        drives.reserve(replicatorEntry.drives().size());
        for (const auto& [driveKey, _]: replicatorEntry.drives()) {
			auto pDrive = utils::GetDrive(
				driveKey,
				m_replicatorKey,
				timestamp,
				*pDriveCacheView,
				*pReplicatorCacheView,
				*pDownloadChannelCacheView);
			if (!pDrive)
				CATAPULT_THROW_RUNTIME_ERROR_1("drive not found", driveKey)
			drives.emplace_back(pDrive);
		}

        return drives;
    }
}}
