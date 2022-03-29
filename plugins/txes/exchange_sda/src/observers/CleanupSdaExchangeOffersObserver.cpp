/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "catapult/cache_core/AccountStateCache.h"

namespace catapult { namespace observers {

    DEFINE_OBSERVER(CleanupSdaOffers, model::BlockNotification<1>, [](const model::BlockNotification<1>& notification, const ObserverContext& context) {
        auto& cache = context.Cache.sub<cache::SdaExchangeCache>();
        auto expiringSdaOfferOwners = cache.expiringOfferOwners(context.Height);
        for (const auto& key : expiringSdaOfferOwners) {
            auto cacheIter = cache.find(key);
            auto* pEntry = cacheIter.tryGet();
            // Entry can not exist because in old versions of blockchain we remove exchange entry
            if (!pEntry)
                continue;

            auto& entry = *pEntry;
            SdaOfferExpiryUpdater sdaOfferExpiryUpdater(cache, entry);

            for (const auto& map :entry.sdaOfferBalances()) {
                Amount amount(0);
                auto onSdaOfferBalancesExpired = [&amount](const state::SdaOfferBalanceMap::const_iterator& iter) {
                    amount = amount + iter->second.CurrentMosaicGet;
                };
                entry.expireOffers(context.Height, onSdaOfferBalancesExpired);

                CreditAccount(entry.owner(), map.first.first, amount, context);
            }
        }

        auto maxRollbackBlocks = context.Config.Network.MaxRollbackBlocks;
		if (context.Height.unwrap() <= maxRollbackBlocks)
		    return;

		auto pruneHeight = Height(context.Height.unwrap() - maxRollbackBlocks);
		expiringSdaOfferOwners = cache.expiringOfferOwners(pruneHeight);
        for (const auto& key : expiringSdaOfferOwners) {
            auto cacheIter = cache.find(key);
            auto* pEntry = cacheIter.tryGet();
            // Entry can not exist because in old versions of blockchain we remove exchange entry
            if (!pEntry)
                continue;

            auto& entry = *pEntry;
            SdaOfferExpiryUpdater offerExpiryUpdater(cache, entry);
            auto expiredOffers = entry.expiredSdaOfferBalances();
            if (expiredOffers.count(pruneHeight)) {
                expiredOffers.at(pruneHeight).clear();
                expiredOffers.erase(pruneHeight);
            }
        }

		auto cleanUpHeight = Height(context.Height.unwrap() - 2 * maxRollbackBlocks);
		expiringSdaOfferOwners = cache.expiringOfferOwners(cleanUpHeight);
		for (const auto& key : expiringSdaOfferOwners) {
		    cache.removeExpiryHeight(key, cleanUpHeight);
	    }
    });
}}