/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once

#include "catapult/cache/CatapultCache.h"
#include "catapult/state/StorageState.h"

namespace catapult { namespace state {

    class StorageStateImpl : public StorageState {
    public:
        explicit StorageStateImpl() = default;

    public:
    	Height getChainHeight() override;

		bool isReplicatorRegistered(const Key& key, const cache::ReadOnlyCatapultCache& cache) override;

        bool driveExists(const Key& driveKey, const cache::ReadOnlyCatapultCache& cache) override;
        Drive getDrive(const Key& driveKey, const cache::ReadOnlyCatapultCache& cache) override;
        bool isReplicatorAssignedToDrive(const Key& key, const Key& driveKey, const cache::ReadOnlyCatapultCache& cache) override;
        bool isReplicatorAssignedToChannel(const Key& key, const Hash256& channelId, const cache::ReadOnlyCatapultCache& cache) override;
        std::vector<Key> getReplicatorDriveKeys(const Key& replicatorKey, const cache::ReadOnlyCatapultCache& cache) override;
        std::vector<Drive> getReplicatorDrives(const Key& replicatorKey, const cache::ReadOnlyCatapultCache& cache) override;
        std::vector<Key> getDriveReplicators(const Key& driveKey, const cache::ReadOnlyCatapultCache& cache) override;
        std::vector<Hash256> getDriveChannels(const Key& driveKey, const cache::ReadOnlyCatapultCache& cache) override;
        std::vector<Key> getDonatorShard(const Key& driveKey, const Key& replicatorKey, const cache::ReadOnlyCatapultCache& cache) override;
        ModificationShard getDonatorShardExtended(const Key& driveKey, const Key& replicatorKey, const cache::ReadOnlyCatapultCache& cache) override;
        std::vector<Key> getRecipientShard(const Key& driveKey, const Key& replicatorKey, const cache::ReadOnlyCatapultCache& cache) override;
//        SizeMap getCumulativeUploadSizesBytes(const Key& driveKey, const Key& replicatorKey) override;

		std::unique_ptr<ApprovedDataModification> getLastApprovedDataModification(const Key& driveKey, const cache::ReadOnlyCatapultCache& cache) override;

		std::vector<CompletedModification> getCompletedModifications(const Key& driveKey, const cache::ReadOnlyCatapultCache& cache) override;

        uint64_t getDownloadWorkBytes(const Key& replicatorKey, const Key& driveKey, const cache::ReadOnlyCatapultCache& cache) override;

        bool downloadChannelExists(const Hash256& id, const cache::ReadOnlyCatapultCache& cache) override;
        std::unique_ptr<DownloadChannel> getDownloadChannel(const Key& replicatorKey, const Hash256& id, const cache::ReadOnlyCatapultCache& cache) override;

		std::optional<DriveVerification> getActiveVerification(const Key& driveKey, const Timestamp& blockTimestamp, const cache::ReadOnlyCatapultCache& cache) override;
		std::set<Hash256> getReplicatorChannelIds(const Key& replicatorKey, const cache::ReadOnlyCatapultCache& cache) override;
    };
}}
