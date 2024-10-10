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
		bool isReplicatorRegistered() override;
        std::shared_ptr<Drive> getDrive(const Key& driveKey, const Timestamp& timestamp) override;
        std::vector<std::shared_ptr<Drive>> getDrives(const Timestamp& timestamp) override;
    };
}}
