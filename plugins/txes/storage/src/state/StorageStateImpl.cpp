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
					modification.ExpectedUploadSizeMegabytes,
					modification.ActualUploadSizeMegabytes });

            return Drive{
				driveEntry.key(),
				driveEntry.owner(),
				driveEntry.size(),
				driveEntry.replicators(),
				dataModifications
            };
        }

		std::optional<DriveVerification> GetActiveVerification(const Key& driveKey, const cache::BcDriveCacheView& driveCacheView, const Timestamp& blockTimestamp) {
			auto driveIter = driveCacheView.find(driveKey);
			const auto& driveEntry = driveIter.get();

			return {};

//			if (driveEntry.verification()) {
//				const auto& verification = *driveEntry.verification();
//				return DriveVerification{driveKey,
//					verification.expired(blockTimestamp),
//					verification.VerificationTrigger,
//				   	driveEntry.rootHash(),
//					verification.Shards};
//			}
//
//			return {};
		}
    }

    Height StorageStateImpl::getChainHeight() {
    	return m_pCache->height();
    }

    bool StorageStateImpl::isReplicatorRegistered(const Key& key) {
		auto pReplicatorCacheView = m_pCache->sub<cache::ReplicatorCache>().createView(m_pCache->height());
		return pReplicatorCacheView->contains(key);
    }

    bool StorageStateImpl::driveExists(const Key& driveKey) {
        auto pDriveCacheView = m_pCache->sub<cache::BcDriveCache>().createView(m_pCache->height());
        return pDriveCacheView->contains(driveKey);
    }

    Drive StorageStateImpl::getDrive(const Key& driveKey) {
        return GetDrive(driveKey, *m_pCache->sub<cache::BcDriveCache>().createView(m_pCache->height()));
    }

    bool StorageStateImpl::isReplicatorAssignedToDrive(const Key& replicatorKey, const Key& driveKey) {
		auto m_pReplicatorCache = m_pCache->sub<cache::ReplicatorCache>().createView(m_pCache->height());
		auto replicatorIter = m_pReplicatorCache->find(replicatorKey);
		const auto* replicatorEntry = replicatorIter.tryGet();
		if (!replicatorEntry) {
			return false;
		}
		const auto& drives = replicatorEntry->drives();
		return drives.find(driveKey) != drives.end();
    }

    bool StorageStateImpl::isReplicatorAssignedToChannel(const Key& replicatorKey, const Hash256& channelId) {
    	auto m_pReplicatorCache = m_pCache->sub<cache::ReplicatorCache>().createView(m_pCache->height());
    	auto replicatorIter = m_pReplicatorCache->find(replicatorKey);
    	const auto* replicatorEntry = replicatorIter.tryGet();
    	if (!replicatorEntry) {
    		return false;
    	}
    	const auto& downloadChannels = replicatorEntry->downloadChannels();
    	return downloadChannels.find(channelId) != downloadChannels.end();
    }

    std::vector<Key> StorageStateImpl::getReplicatorDriveKeys(const Key& replicatorKey) {
    	auto pReplicatorCacheView = m_pCache->sub<cache::ReplicatorCache>().createView(m_pCache->height());
    	auto pDriveCacheView = m_pCache->sub<cache::BcDriveCache>().createView(m_pCache->height());

    	auto replicatorIter = pReplicatorCacheView->find(replicatorKey);
    	const auto& replicatorEntry = replicatorIter.get();

    	std::vector<Key> drives;
    	drives.reserve(replicatorEntry.drives().size());
    	for (const auto& [key, _]: replicatorEntry.drives())
    		drives.emplace_back(key);

    	return drives;
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

    std::vector<Hash256> StorageStateImpl::getDriveChannels(const Key& driveKey) {
    	auto pDriveCacheView = m_pCache->sub<cache::BcDriveCache>().createView(m_pCache->height());
    	auto driveIter = pDriveCacheView->find(driveKey);
    	const auto& driveEntry = driveIter.get();

    	std::vector<Hash256> downloadChannels(driveEntry.downloadShards().begin(), driveEntry.downloadShards().end());
		return downloadChannels;
	}
	std::set<Hash256> StorageStateImpl::getReplicatorChannelIds(const Key& replicatorKey) {
    	auto pReplicatorCacheView = m_pCache->sub<cache::ReplicatorCache>().createView(m_pCache->height());
    	auto replicatorIter = pReplicatorCacheView->find(replicatorKey);
		const auto& replicatorEntry = replicatorIter.get();

		return replicatorEntry.downloadChannels();
	}

	std::vector<Key> StorageStateImpl::getDriveReplicators(const Key& driveKey) {
        auto drive = GetDrive(driveKey, *m_pCache->sub<cache::BcDriveCache>().createView(m_pCache->height()));
        return {drive.Replicators.begin(), drive.Replicators.end()};
    }

    std::vector<Key> StorageStateImpl::getDonatorShard(const Key& driveKey, const Key& replicatorKey) {
    	auto pDriveCacheView = m_pCache->sub<cache::BcDriveCache>().createView(m_pCache->height());
    	auto driveIter = pDriveCacheView->find(driveKey);
    	const auto& driveEntry = driveIter.get();
    	const auto& shard = driveEntry.dataModificationShards().at(replicatorKey);
    	std::vector<Key> keys;
		for (const auto& [key, _]: shard.m_actualShardMembers) {
			keys.push_back(key);
		}
		return keys;
	}

	ModificationShard StorageStateImpl::getDonatorShardExtended(const Key& driveKey, const Key& replicatorKey) {
    	auto pDriveCacheView = m_pCache->sub<cache::BcDriveCache>().createView(m_pCache->height());
    	auto driveIter = pDriveCacheView->find(driveKey);
    	const auto& driveEntry = driveIter.get();
    	const auto& shard = driveEntry.dataModificationShards().at(replicatorKey);

    	return {shard.m_actualShardMembers, shard.m_formerShardMembers, shard.m_ownerUpload};
    }

	std::vector<Key> StorageStateImpl::getRecipientShard(const Key& driveKey, const Key& replicatorKey) {
		auto pDriveCacheView = m_pCache->sub<cache::BcDriveCache>().createView(m_pCache->height());
		auto driveIter = pDriveCacheView->find(driveKey);
		const auto& driveEntry = driveIter.get();

		std::vector<Key> donatorShard;

		for (const auto& [key, shard]: driveEntry.dataModificationShards()){
			const auto& actualShard = shard.m_actualShardMembers;
			if (actualShard.find(replicatorKey) != actualShard.end())
			{
				donatorShard.push_back(key);
			}
		}

		return donatorShard;
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
			completedModificationsIter->ExpectedUploadSizeMegabytes,
			completedModificationsIter->ActualUploadSizeMegabytes,
			signers,
										   driveEntry.usedSizeBytes()});
    }

    uint64_t StorageStateImpl::getDownloadWorkBytes(const Key& replicatorKey, const Key& driveKey) {
        auto pReplicatorCacheView = m_pCache->sub<cache::ReplicatorCache>().createView(m_pCache->height());
        auto replicatorIter = pReplicatorCacheView->find(replicatorKey);
        const auto& replicatorEntry = replicatorIter.get();

        return replicatorEntry.drives().find(driveKey)->second.LastCompletedCumulativeDownloadWorkBytes;
    }

    bool StorageStateImpl::downloadChannelExists(const Hash256& id) {
        auto pDownloadChannelCacheView = m_pCache->sub<cache::DownloadChannelCache>().createView(m_pCache->height());
        return pDownloadChannelCacheView->contains(id);
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
			consumers,
			replicators,
			channelEntry.drive(),
			channelEntry.downloadApprovalInitiationEvent(),
		});
    }

	std::optional<DriveVerification> StorageStateImpl::getActiveVerification(const Key& driveKey, const Timestamp& blockTimestamp) {
		auto pDriveCacheView = m_pCache->sub<cache::BcDriveCache>().createView(m_pCache->height());
		return GetActiveVerification(driveKey, *pDriveCacheView, blockTimestamp);
	}
}}
