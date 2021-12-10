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
        struct MockStorageState : public state::StorageState {
            ~MockStorageState() override = default;

            bool isReplicatorRegistered(const Key& key) override {
                    return false;
            }

            std::vector<Key> getAllReplicators() override {
                    return std::vector<Key>();
            }

            bool driveExist(const Key& driveKey) override {
                    return false;
            }

            state::Drive getDrive(const Key& driveKey) override {
                    return state::Drive();
            }

            bool isReplicatorBelongToDrive(const Key& key, const Key& driveKey) override {
                    return false;
            }

            std::vector<state::Drive> getReplicatorDrives(const Key& replicatorKey) override {
                    return std::vector<state::Drive>();
            }

            std::vector<Key> getDriveReplicators(const Key& driveKey) override {
                    return std::vector<Key>();
            }

            state::DataModification getLastApprovedDataModification(const Key& driveKey) override {
                    return state::DataModification();
            }

            uint64_t getDownloadWork(const Key& replicatorKey, const Key& driveKey) override {
                    return 0;
            }

            bool downloadChannelExist(const Hash256& id) override {
                    return false;
            }

            std::vector<state::DownloadChannel> getDownloadChannels() override {
                    return std::vector<state::DownloadChannel>();
            }

            state::DownloadChannel getDownloadChannel(const Hash256& id) override {
                    return state::DownloadChannel();
            }
        };

#pragma pack(pop)
}}
