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
        Drive GetDrive(const Key& driveKey, const cache::BcDriveCacheTypes::CacheReadOnlyType& driveCache) {
            auto driveIter = driveCache.find(driveKey);
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
					modification.ActualUploadSizeMegabytes,
					modification.FolderName,
					modification.ReadyForApproval,
					modification.IsStream });

			return Drive{
				driveEntry.key(),
				driveEntry.owner(),
				driveEntry.rootHash(),
				driveEntry.size(),
				driveEntry.replicators(),
				dataModifications
            };
        }

		std::optional<DriveVerification> GetActiveVerification(
				const Key& driveKey,
				const cache::BcDriveCacheTypes::CacheReadOnlyType& driveCache,
				const Timestamp& blockTimestamp) {
			auto driveIter = driveCache.find(driveKey);
			const auto& driveEntry = driveIter.get();

			if (driveEntry.verification()) {

				const auto& verification = *driveEntry.verification();
				return DriveVerification{driveKey,
				    verification.Duration,
					verification.expired(blockTimestamp),
					verification.VerificationTrigger,
				   	driveEntry.lastModificationId(),
					verification.Shards};
			}

			return {};
		}
    }

    Height StorageStateImpl::getChainHeight() {
    	return Height();	// TODO: Temporary stub.
    }

    bool StorageStateImpl::isReplicatorRegistered(const Key& key, const cache::ReadOnlyCatapultCache& cache) {
		const auto& replicatorCache = cache.sub<cache::ReplicatorCache>();
		return replicatorCache.contains(key);
    }

    bool StorageStateImpl::driveExists(const Key& driveKey, const cache::ReadOnlyCatapultCache& cache) {
		const auto& driveCache = cache.sub<cache::BcDriveCache>();
        return driveCache.contains(driveKey);
    }

    Drive StorageStateImpl::getDrive(const Key& driveKey, const cache::ReadOnlyCatapultCache& cache) {
        return GetDrive(driveKey, cache.sub<cache::BcDriveCache>());
    }

    bool StorageStateImpl::isReplicatorAssignedToDrive(const Key& replicatorKey, const Key& driveKey, const cache::ReadOnlyCatapultCache& cache) {
		const auto& replicatorCache = cache.sub<cache::ReplicatorCache>();
		auto replicatorIter = replicatorCache.find(replicatorKey);
		const auto* replicatorEntry = replicatorIter.tryGet();
		if (!replicatorEntry) {
			return false;
		}
		const auto& drives = replicatorEntry->drives();
		return drives.find(driveKey) != drives.end();
    }

    bool StorageStateImpl::isReplicatorAssignedToChannel(const Key& replicatorKey, const Hash256& channelId, const cache::ReadOnlyCatapultCache& cache) {
		const auto& replicatorCache = cache.sub<cache::ReplicatorCache>();
		auto replicatorIter = replicatorCache.find(replicatorKey);
    	const auto* replicatorEntry = replicatorIter.tryGet();
    	if (!replicatorEntry) {
    		return false;
    	}
    	const auto& downloadChannels = replicatorEntry->downloadChannels();
    	return downloadChannels.find(channelId) != downloadChannels.end();
    }

    std::vector<Key> StorageStateImpl::getReplicatorDriveKeys(const Key& replicatorKey, const cache::ReadOnlyCatapultCache& cache) {
		const auto& replicatorCache = cache.sub<cache::ReplicatorCache>();
		auto replicatorIter = replicatorCache.find(replicatorKey);
		const auto& replicatorEntry = replicatorIter.get();

    	std::vector<Key> drives;
    	drives.reserve(replicatorEntry.drives().size());
    	for (const auto& [key, _]: replicatorEntry.drives())
    		drives.emplace_back(key);

    	return drives;
	}

    std::vector<Drive> StorageStateImpl::getReplicatorDrives(const Key& replicatorKey, const cache::ReadOnlyCatapultCache& cache) {
		const auto& replicatorCache = cache.sub<cache::ReplicatorCache>();
		const auto& driveCache = cache.sub<cache::BcDriveCache>();

        auto replicatorIter = replicatorCache.find(replicatorKey);
		const auto& replicatorEntry = replicatorIter.get();

        std::vector<Drive> drives;
        drives.reserve(replicatorEntry.drives().size());
        for (const auto& pair: replicatorEntry.drives())
            drives.emplace_back(GetDrive(pair.first, driveCache));

        return drives;
    }

    std::vector<Hash256> StorageStateImpl::getDriveChannels(const Key& driveKey, const cache::ReadOnlyCatapultCache& cache) {
		const auto& driveCache = cache.sub<cache::BcDriveCache>();
		auto driveIter = driveCache.find(driveKey);
    	const auto& driveEntry = driveIter.get();

    	std::vector<Hash256> downloadChannels(driveEntry.downloadShards().begin(), driveEntry.downloadShards().end());
		return downloadChannels;
	}
	std::set<Hash256> StorageStateImpl::getReplicatorChannelIds(const Key& replicatorKey, const cache::ReadOnlyCatapultCache& cache) {
		const auto& replicatorCache = cache.sub<cache::ReplicatorCache>();
		auto replicatorIter = replicatorCache.find(replicatorKey);
		const auto& replicatorEntry = replicatorIter.get();

		return replicatorEntry.downloadChannels();
	}

	std::vector<Key> StorageStateImpl::getDriveReplicators(const Key& driveKey, const cache::ReadOnlyCatapultCache& cache) {
        auto drive = GetDrive(driveKey, cache.sub<cache::BcDriveCache>());
        return {drive.Replicators.begin(), drive.Replicators.end()};
    }

    std::vector<Key> StorageStateImpl::getDonatorShard(const Key& driveKey, const Key& replicatorKey, const cache::ReadOnlyCatapultCache& cache) {
		const auto& driveCache = cache.sub<cache::BcDriveCache>();
		auto driveIter = driveCache.find(driveKey);
    	const auto& driveEntry = driveIter.get();
    	const auto& shard = driveEntry.dataModificationShards().at(replicatorKey);
    	std::vector<Key> keys;
		for (const auto& [key, _]: shard.ActualShardMembers) {
			keys.push_back(key);
		}
		return keys;
	}

	ModificationShard StorageStateImpl::getDonatorShardExtended(const Key& driveKey, const Key& replicatorKey, const cache::ReadOnlyCatapultCache& cache) {
		const auto& driveCache = cache.sub<cache::BcDriveCache>();
    	auto driveIter = driveCache.find(driveKey);
    	const auto& driveEntry = driveIter.get();
    	const auto& shard = driveEntry.dataModificationShards().at(replicatorKey);

    	return {shard.ActualShardMembers, shard.FormerShardMembers, shard.OwnerUpload };
    }

	std::vector<Key> StorageStateImpl::getRecipientShard(const Key& driveKey, const Key& replicatorKey, const cache::ReadOnlyCatapultCache& cache) {
		const auto& driveCache = cache.sub<cache::BcDriveCache>();
		auto driveIter = driveCache.find(driveKey);
		const auto& driveEntry = driveIter.get();

		std::vector<Key> donatorShard;

		for (const auto& [key, shard]: driveEntry.dataModificationShards()){
			const auto& actualShard = shard.ActualShardMembers;
			if (actualShard.find(replicatorKey) != actualShard.end())
			{
				donatorShard.push_back(key);
			}
		}

		return donatorShard;
	}

	std::unique_ptr<ApprovedDataModification> StorageStateImpl::getLastApprovedDataModification(const Key& driveKey, const cache::ReadOnlyCatapultCache& cache) {
		const auto& driveCache = cache.sub<cache::BcDriveCache>();
        auto driveIter = driveCache.find(driveKey);
		const auto& driveEntry = driveIter.get();

        auto completedModificationsIter = driveEntry.completedDataModifications().rbegin();
        while (completedModificationsIter != driveEntry.completedDataModifications().rend()) {
            if (completedModificationsIter->ApprovalState == state::DataModificationApprovalState::Approved)
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
			completedModificationsIter->FolderName,
			completedModificationsIter->ReadyForApproval,
			completedModificationsIter->IsStream,
			signers,
		    driveEntry.usedSizeBytes()});
    }

    std::vector<CompletedModification> StorageStateImpl::getCompletedModifications(const Key& driveKey, const cache::ReadOnlyCatapultCache& cache) {
		const auto& driveCache = cache.sub<cache::BcDriveCache>();
    	auto driveIter = driveCache.find(driveKey);
    	const auto& driveEntry = driveIter.get();

		std::vector<CompletedModification> completedModifications;

    	for( const auto& modification: driveEntry.completedDataModifications() ) {
			CompletedModification completedModification;
			completedModification.ModificationId = modification.Id;
			if ( modification.ApprovalState == DataModificationApprovalState::Cancelled ) {
				completedModification.Status = CompletedModification::CompletionStatus::CANCELLED;
			}
			else {
				completedModification.Status = CompletedModification::CompletionStatus::APPROVED;
			}
			completedModifications.push_back(completedModification);
		}
		return completedModifications;
    }

    uint64_t StorageStateImpl::getDownloadWorkBytes(const Key& replicatorKey, const Key& driveKey, const cache::ReadOnlyCatapultCache& cache) {
		const auto& replicatorCache = cache.sub<cache::ReplicatorCache>();
		auto replicatorIter = replicatorCache.find(replicatorKey);
        const auto& replicatorEntry = replicatorIter.get();

        return replicatorEntry.drives().find(driveKey)->second.LastCompletedCumulativeDownloadWorkBytes;
    }

    bool StorageStateImpl::downloadChannelExists(const Hash256& id, const cache::ReadOnlyCatapultCache& cache) {
		const auto& downloadChannelCache = cache.sub<cache::DownloadChannelCache>();
		return downloadChannelCache.contains(id);
    }

	std::unique_ptr<DownloadChannel> StorageStateImpl::getDownloadChannel(const Key& replicatorKey, const Hash256& id, const cache::ReadOnlyCatapultCache& cache) {
		const auto& downloadChannelCache = cache.sub<cache::DownloadChannelCache>();
		auto channelIter = downloadChannelCache.find(id);
        const auto& channelEntry = channelIter.get();

        const auto& replicators = channelEntry.shardReplicators();
        if (replicators.find(replicatorKey) == replicators.end())
			return nullptr;

        auto consumers = channelEntry.listOfPublicKeys();
        consumers.emplace_back(channelEntry.consumer());

        return std::make_unique<DownloadChannel>(DownloadChannel{
			channelEntry.id(),
			channelEntry.downloadSize(),
			consumers,
			{replicators.begin(), replicators.end()},
			channelEntry.drive(),
			channelEntry.downloadApprovalInitiationEvent(),
		});
    }

	std::optional<DriveVerification> StorageStateImpl::getActiveVerification(const Key& driveKey, const Timestamp& blockTimestamp, const cache::ReadOnlyCatapultCache& cache) {
		const auto& driveCache = cache.sub<cache::BcDriveCache>();
		return GetActiveVerification(driveKey, driveCache, blockTimestamp);
	}
}}
