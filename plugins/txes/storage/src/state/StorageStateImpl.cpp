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
            for (const auto& modification: activeDataModifications)
                dataModifications.emplace_back(DataModification{
					modification.Id,
					modification.Owner,
					driveKey,
					modification.DownloadDataCdi,
					modification.ExpectedUploadSize,
					modification.ActualUploadSize});

            return Drive{
				driveEntry.key(),
				driveEntry.owner(),
				driveEntry.size(),
				driveEntry.replicators(),
				dataModifications
            };
        }
    }

    bool StorageStateImpl::isReplicatorRegistered(const Key& key) {
        const auto& keys = m_pKeyCollector->keys();
        return (keys.find(key) != keys.end());
    }

    bool StorageStateImpl::driveExist(const Key& driveKey) {
        auto driveCacheView = m_pCache->sub<cache::BcDriveCache>().createView(m_pCache->height());
        return driveCacheView->contains(driveKey);
    }

    Drive StorageStateImpl::getDrive(const Key& driveKey) {
        return GetDrive(driveKey, *m_pCache->sub<cache::BcDriveCache>().createView(m_pCache->height()));
    }

    bool StorageStateImpl::isReplicatorAssignedToDrive(const Key& key, const Key& driveKey) {
		const auto& pDriveCacheView = *m_pCache->sub<cache::BcDriveCache>().createView(m_pCache->height());
		auto driveIter = pDriveCacheView.find(driveKey);
		auto driveEntry = driveIter.get();
        return driveEntry.replicators().find(key) != driveEntry.replicators().end();
    }

    std::vector<Drive> StorageStateImpl::getReplicatorDrives(const Key& replicatorKey) {
        auto pReplicatorCacheView = m_pCache->sub<cache::ReplicatorCache>().createView(m_pCache->height());
        auto pDriveCacheView = m_pCache->sub<cache::BcDriveCache>().createView(m_pCache->height());

        auto replicatorIter = pReplicatorCacheView->find(replicatorKey);
        auto replicatorEntry = replicatorIter.get();

        std::vector<Drive> drives;
        drives.reserve(replicatorEntry.drives().size());
        for (const auto& pair: replicatorEntry.drives())
            drives.emplace_back(GetDrive(pair.first, *pDriveCacheView));

        return drives;
    }

    std::vector<Key> StorageStateImpl::getDriveReplicators(const Key& driveKey) {
        auto drive = GetDrive(driveKey, *m_pCache->sub<cache::BcDriveCache>().createView(m_pCache->height()));
        return {drive.Replicators.begin(), drive.Replicators.end()};
    }

	std::unique_ptr<ApprovedDataModification> StorageStateImpl::getLastApprovedDataModification(const Key& driveKey) {
        auto driveCacheView = m_pCache->sub<cache::BcDriveCache>().createView(m_pCache->height());
        auto driveIter = driveCacheView->find(driveKey);
        auto driveEntry = driveIter.get();

        auto completedModificationsIter = driveEntry.completedDataModifications().rbegin();
        while (completedModificationsIter != driveEntry.completedDataModifications().rend()) {
            if (completedModificationsIter->State == state::DataModificationState::Succeeded)
                break;

            ++completedModificationsIter;
        }

        if (completedModificationsIter == driveEntry.completedDataModifications().rend())
            return nullptr;

        std::vector<Key> signers;
        for (const auto& state : driveEntry.confirmedStates()) {
            if (state.second == completedModificationsIter->Id)
                signers.emplace_back(state.first);
        }

        return std::make_unique<ApprovedDataModification>(ApprovedDataModification{
			completedModificationsIter->Id,
			driveEntry.owner(),
			driveEntry.key(),
			completedModificationsIter->DownloadDataCdi,
			completedModificationsIter->ExpectedUploadSize,
			completedModificationsIter->ActualUploadSize,
			signers,
			driveEntry.usedSize()});
    }

    uint64_t StorageStateImpl::getDownloadWork(const Key& replicatorKey, const Key& driveKey) {
        auto pReplicatorCacheView = m_pCache->sub<cache::ReplicatorCache>().createView(m_pCache->height());
        auto replicatorIter = pReplicatorCacheView->find(replicatorKey);
        auto replicatorEntry = replicatorIter.get();

        return replicatorEntry.drives().find(driveKey)->second.LastCompletedCumulativeDownloadWork;
    }

    bool StorageStateImpl::downloadChannelExist(const Hash256& id) {
        auto downloadChannelCacheView = m_pCache->sub<cache::DownloadChannelCache>().createView(m_pCache->height());
        return downloadChannelCacheView->contains(id);
    }

    std::vector<DownloadChannel> StorageStateImpl::getDownloadChannels() {
        auto downloadChannelCacheView = m_pCache->sub<cache::DownloadChannelCache>().createView(m_pCache->height());
        auto iterableView = downloadChannelCacheView->tryMakeIterableView();

        std::vector<DownloadChannel> channels;
        channels.reserve(downloadChannelCacheView->size());
        for (auto it = iterableView->begin(); it != iterableView->end(); ++it) {
            auto consumers = it->second.listOfPublicKeys();
            consumers.emplace_back(it->second.consumer());
            channels.emplace_back(DownloadChannel{
                    it->first,
                    it->second.downloadSize(),
                    it->second.downloadApprovalCount(),
                    consumers
            });
        }

        return channels;
    }

    DownloadChannel StorageStateImpl::getDownloadChannel(const Hash256& id) {
        auto downloadChannelCacheView = m_pCache->sub<cache::DownloadChannelCache>().createView(m_pCache->height());

        auto channelIter = downloadChannelCacheView->find(id);
        auto channelEntry = channelIter.get();

        auto consumers = channelEntry.listOfPublicKeys();
        consumers.emplace_back(channelEntry.consumer());

        return DownloadChannel{
			channelEntry.id(),
			channelEntry.downloadSize(),
			channelEntry.downloadApprovalCount(),
			consumers,
			Key()}; // TODO set real key
    }

	std::unique_ptr<DriveVerification> StorageStateImpl::getActiveVerification(const Key& driveKey) {
        auto driveCacheView = m_pCache->sub<cache::BcDriveCache>().createView(m_pCache->height());
        auto driveIter = driveCacheView->find(driveKey);
        auto driveEntry = driveIter.get();

        if (driveEntry.verifications().size() > 0) {
			const auto& verification = driveEntry.verifications()[0];
			return std::make_unique<DriveVerification>(DriveVerification{
				verification.VerificationTrigger,
				driveEntry.rootHash(),
				verification.Shards});
		}

		return nullptr;
	}
}}
