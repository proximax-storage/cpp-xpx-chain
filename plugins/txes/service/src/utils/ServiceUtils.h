
/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/types.h"
#include "catapult/utils/ArraySet.h"
#include "src/state/DriveEntry.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "catapult/cache/CatapultCacheDelta.h"
#include "catapult/cache/ReadOnlyCatapultCache.h"

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

    template<typename TCache>
    inline Amount GetBillingBalanceOfDrive(const state::DriveEntry& driveEntry, const TCache& cache, const MosaicId& storageMosaicId) {
        const auto& accountCache = cache.template sub<cache::AccountStateCache>();
        auto accountIter = accountCache.find(driveEntry.key());
        const auto& driveAccount = accountIter.get();

        auto balance = driveAccount.Balances.get(storageMosaicId);
        auto deposit = Amount(utils::CalculateDriveDeposit(driveEntry).unwrap() * driveEntry.replicators().size());
        if (balance < deposit)
            CATAPULT_THROW_RUNTIME_ERROR("Got unexpected state: drive doesn't contain deposit");

        return balance - deposit;
    }

}}