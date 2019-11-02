
/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/types.h"
#include "catapult/utils/ArraySet.h"
#include "src/state/DriveEntry.h"

namespace catapult { namespace utils {

    inline Amount CalculateDriveDeposit(const state::DriveEntry& driveEntry) {
        return Amount(driveEntry.size());
    }

    inline Amount CalculateFileDeposit(const state::DriveEntry& driveEntry, const Hash256& fileHash) {
        return Amount(driveEntry.files().at(fileHash).Size);
    }

    inline Amount CalculateFileUpload(const state::DriveEntry& driveEntry, const uint64_t& size) {
        return Amount((driveEntry.replicas() - 1) * size);
    }

}}