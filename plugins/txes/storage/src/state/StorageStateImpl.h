/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once

#include "catapult/cache/CatapultCache.h"
#include "catapult/state/StorageState.h"
#include "src/cache/ReplicatorKeyCollector.h"

namespace catapult { namespace state {

    class StorageStateImpl : public StorageState {
    public:
        explicit StorageStateImpl(std::shared_ptr<cache::ReplicatorKeyCollector> pKeyCollector)
                : m_pKeyCollector(std::move(pKeyCollector)) {}

    public:
    	Height getChainHeight() override;

		bool isReplicatorRegistered(const Key& key) override;

        bool driveExists(const Key& driveKey) override;
        Drive getDrive(const Key& driveKey) override;
        bool isReplicatorAssignedToDrive(const Key& key, const Key& driveKey) override;
        bool isReplicatorAssignedToChannel(const Key& key, const Hash256& channelId) override;
        std::vector<Key> getReplicatorDriveKeys(const Key& replicatorKey) override;
        std::vector<Drive> getReplicatorDrives(const Key& replicatorKey) override;
        std::vector<Key> getDriveReplicators(const Key& driveKey) override;
        std::vector<Hash256> getDriveChannels(const Key& driveKey) override;
        std::vector<Key> getDonatorShard(const Key& driveKey, const Key& replicatorKey) override;
        std::vector<Key> getRecipientShard(const Key& driveKey, const Key& replicatorKey) override;

		std::unique_ptr<ApprovedDataModification> getLastApprovedDataModification(const Key& driveKey) override;

        uint64_t getDownloadWorkBytes(const Key& replicatorKey, const Key& driveKey) override;

        bool downloadChannelExists(const Hash256& id) override;
        std::vector<DownloadChannel> getDownloadChannels(const Key& replicatorKey) override;
		std::unique_ptr<DownloadChannel> getDownloadChannel(const Key& replicatorKey, const Hash256& id) override;

		virtual std::unique_ptr<DriveVerification> getActiveVerification(const Key& driveKey) override;

    private:
        std::shared_ptr<cache::ReplicatorKeyCollector> m_pKeyCollector;
    };
}}
