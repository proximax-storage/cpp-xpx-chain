/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once

#include "catapult/state/StorageState.h"

namespace catapult { namespace mocks {

#pragma pack(push, 1)

    /// Mock storageState.
    class MockStorageState : public state::StorageState {
    public:
        ~MockStorageState() override = default;

    public:

        bool isReplicatorRegistered(const Key& key) override {
            return false;
        }

        bool driveExist(const Key& driveKey) override {
            return false;
        }

        state::Drive getDrive(const Key& driveKey) override {
            return state::Drive();
        }

        bool isReplicatorAssignedToDrive(const Key& key, const Key& driveKey) override {
            return false;
        }

        std::vector<state::Drive> getReplicatorDrives(const Key& replicatorKey) override {
            return std::vector<state::Drive>();
        }

        std::vector<Key> getDriveReplicators(const Key& driveKey) override {
            return std::vector<Key>();
        }

		std::unique_ptr<state::ApprovedDataModification> getLastApprovedDataModification(const Key& driveKey) override {
            return nullptr;
        }

        uint64_t getDownloadWorkBytes(const Key& replicatorKey, const Key& driveKey) override {
            return 0;
        }

        bool downloadChannelExist(const Hash256& id) override {
            return false;
        }

        std::vector<state::DownloadChannel> getDownloadChannels(const Key&) override {
            return std::vector<state::DownloadChannel>();
        }

		std::unique_ptr<state::DownloadChannel> getDownloadChannel(const Key& replicatorKey, const Hash256& id) override {
            return nullptr;
        }

		virtual std::unique_ptr<state::DriveVerification> getActiveVerification(const Key& driveKey) override {
            return nullptr;
        }

		std::vector<state::DriveVerification> getActiveVerifications(const Key& replicatorKey) {
			return {};
		}

		std::vector<Key> getDonatorShard(const Key& driveKey, const Key& replicatorKey) override {
			return std::vector<Key>();
		}

		std::vector<Key> getRecipientShard(const Key& driveKey, const Key& replicatorKey) override {
			return std::vector<Key>();
		}
	};

#pragma pack(pop)
}}
