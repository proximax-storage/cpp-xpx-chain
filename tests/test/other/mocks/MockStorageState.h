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

        bool isReplicatorRegistered() override {
            return false;
        }

        std::shared_ptr<state::Drive> getDrive(const Key& driveKey, const Timestamp& timestamp) override {
            return nullptr;
        }

		std::vector<std::shared_ptr<state::Drive>> getDrives(const Timestamp& timestamp) override {
            return {};
        }
	};

#pragma pack(pop)
}}
