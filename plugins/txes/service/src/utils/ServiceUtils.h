
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
#include "catapult/model/NotificationSubscriber.h"

namespace catapult { namespace utils {

    inline Amount CalculateDriveDeposit(const state::DriveEntry& driveEntry) {
        return Amount(driveEntry.size());
    }

    inline Amount CalculateFileDeposit(const uint64_t& size) {
        return Amount(size);
    }

    inline Amount CalculateFileDeposit(const state::DriveEntry& driveEntry, const Hash256& fileHash) {
        return Amount(driveEntry.files().at(fileHash).Size);
    }

    inline Amount CalculateFileUpload(const state::DriveEntry& driveEntry, const uint64_t& size) {
        return Amount(driveEntry.replicas() * size);
    }

    inline Amount CalculateFileDownload(const uint64_t& size) {
        return Amount(size);
    }

    inline Amount CalculateFileDownload(const model::DownloadAction* pFile, uint16_t fileCount) {
		uint64_t totalSize = 0u;
		for (auto i = 0u; i < fileCount; ++i, ++pFile) {
			totalSize += pFile->FileSize;
		}
        return Amount(totalSize);
    }

    template<typename TCache>
    inline Amount GetDriveBalance(const state::DriveEntry& driveEntry, const TCache& cache, const MosaicId& mosaicId) {
        const auto& accountCache = cache.template sub<cache::AccountStateCache>();
        auto accountIter = accountCache.find(driveEntry.key());
        const auto& driveAccount = accountIter.get();
        return driveAccount.Balances.get(mosaicId);
    }

	enum class Operation : uint8_t {
		// Buy service mosaics
		Buy,
		// Sell service mosaics
		Sell,
	};

	inline void SwapMosaics(
			const Key& account,
			const std::vector<model::UnresolvedMosaic>& mosaics,
			model::NotificationSubscriber& sub,
			const config::ImmutableConfiguration& immutableCfg,
			Operation operation) {
		auto currencyMosaicId = config::GetUnresolvedCurrencyMosaicId(immutableCfg);
		for (auto& mosaic : mosaics) {
			switch (operation) {
			case Operation::Buy:
				sub.notify(model::BalanceDebitNotification<1>(account, currencyMosaicId, mosaic.Amount));
				sub.notify(model::BalanceCreditNotification<1>(account, mosaic.MosaicId, mosaic.Amount));
				break;
			case Operation::Sell:
				sub.notify(model::BalanceDebitNotification<1>(account, mosaic.MosaicId, mosaic.Amount));
				sub.notify(model::BalanceCreditNotification<1>(account, currencyMosaicId, mosaic.Amount));
				break;
			default:
				CATAPULT_THROW_INVALID_ARGUMENT_1("unsupported operation", operation);
			}
		}
	}
}}
