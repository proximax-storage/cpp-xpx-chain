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
        Drive GetDrive(const Key& driveKey, const cache::BcDriveCacheView& driveCacheView) {
            auto driveIter = driveCacheView.find(driveKey);
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
        auto pDriveCacheView = m_pCache->sub<cache::BcDriveCache>().createView(m_pCache->height());
        return pDriveCacheView->contains(driveKey);
    }

    Drive StorageStateImpl::getDrive(const Key& driveKey) {
        return GetDrive(driveKey, *m_pCache->sub<cache::BcDriveCache>().createView(m_pCache->height()));
    }

    bool StorageStateImpl::isReplicatorAssignedToDrive(const Key& key, const Key& driveKey) {
		auto pDriveCacheView = m_pCache->sub<cache::BcDriveCache>().createView(m_pCache->height());
		auto driveIter = pDriveCacheView->find(driveKey);
		const auto& driveEntry = driveIter.get();
        return driveEntry.replicators().find(key) != driveEntry.replicators().end();
    }

    std::vector<Drive> StorageStateImpl::getReplicatorDrives(const Key& replicatorKey) {
        auto pReplicatorCacheView = m_pCache->sub<cache::ReplicatorCache>().createView(m_pCache->height());
        auto pDriveCacheView = m_pCache->sub<cache::BcDriveCache>().createView(m_pCache->height());

        auto replicatorIter = pReplicatorCacheView->find(replicatorKey);
		const auto& replicatorEntry = replicatorIter.get();

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
        auto pDriveCacheView = m_pCache->sub<cache::BcDriveCache>().createView(m_pCache->height());
        auto driveIter = pDriveCacheView->find(driveKey);
		const auto& driveEntry = driveIter.get();

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
        const auto& replicatorEntry = replicatorIter.get();

        return replicatorEntry.drives().find(driveKey)->second.LastCompletedCumulativeDownloadWork;
    }

    bool StorageStateImpl::downloadChannelExist(const Hash256& id) {
        auto pDownloadChannelCacheView = m_pCache->sub<cache::DownloadChannelCache>().createView(m_pCache->height());
        return pDownloadChannelCacheView->contains(id);
    }

    std::vector<DownloadChannel> StorageStateImpl::getDownloadChannels(const Key& replicatorKey) {
        auto pReplicatorCacheView = m_pCache->sub<cache::ReplicatorCache>().createView(m_pCache->height());
        auto replicatorIter = pReplicatorCacheView->find(replicatorKey);
        const auto& replicatorEntry = replicatorIter.get();

        auto pDownloadChannelCacheView = m_pCache->sub<cache::DownloadChannelCache>().createView(m_pCache->height());

        std::vector<DownloadChannel> channels;
        channels.reserve(replicatorEntry.downloadChannels().size());
        for (const auto& id : replicatorEntry.downloadChannels()) {
			auto channelIter = pDownloadChannelCacheView->find(id);
			const auto& channelEntry = channelIter.get();
            auto consumers = channelEntry.listOfPublicKeys();
            consumers.emplace_back(channelEntry.consumer());
            channels.emplace_back(DownloadChannel{
				id,
				channelEntry.downloadSize(),
				channelEntry.downloadApprovalCount(),
				consumers});
        }

        return channels;
    }

	std::unique_ptr<DownloadChannel> StorageStateImpl::getDownloadChannel(const Key& replicatorKey, const Hash256& id) {
        auto pDownloadChannelCacheView = m_pCache->sub<cache::DownloadChannelCache>().createView(m_pCache->height());
        auto channelIter = pDownloadChannelCacheView->find(id);
        const auto& channelEntry = channelIter.get();

		const auto& cumulativePayments = channelEntry.cumulativePayments();
		if (cumulativePayments.find(replicatorKey) == cumulativePayments.end())
			return nullptr;

		std::vector<Key> replicators;
		replicators.reserve(cumulativePayments.size());
		for (const auto& pair : cumulativePayments)
			replicators.emplace_back(pair.first);

        auto consumers = channelEntry.listOfPublicKeys();
        consumers.emplace_back(channelEntry.consumer());

        return std::make_unique<DownloadChannel>(DownloadChannel{
			channelEntry.id(),
			channelEntry.downloadSize(),
			channelEntry.downloadApprovalCount(),
			consumers,
			replicators,
			channelEntry.drive()});
    }

	std::unique_ptr<DriveVerification> StorageStateImpl::getActiveVerification(const Key& driveKey) {
        auto pDriveCacheView = m_pCache->sub<cache::BcDriveCache>().createView(m_pCache->height());
        auto driveIter = pDriveCacheView->find(driveKey);
        const auto& driveEntry = driveIter.get();

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
