/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "catapult/cache_core/AccountStateCache.h"

namespace catapult { namespace observers {

    using Notification = model::BlockNotification<1>;

    DECLARE_OBSERVER(CleanupSdaOffers, Notification)() {
        return MAKE_OBSERVER(CleanupSdaOffers, Notification, ([](const model::BlockNotification<1>& notification, const ObserverContext& context) {
            auto& cache = context.Cache.sub<cache::SdaExchangeCache>();
            auto expiringSdaOfferOwners = cache.expiringOfferOwners(context.Height);
            for (const auto& key : expiringSdaOfferOwners) {
                auto cacheIter = cache.find(key);
                auto& entry = cacheIter.get();
                SdaOfferExpiryUpdater sdaOfferExpiryUpdater(cache, entry);

                auto onSdaOfferBalancesExpired = [&entry, &context](const state::SdaOfferBalanceMap::const_iterator& iter) {
                    CreditAccount(entry.owner(), iter->first.first, iter->second.CurrentMosaicGive, context);
                };

                entry.expireOffers(context.Height, onSdaOfferBalancesExpired);

                for (auto& expiredPair : entry.expiredSdaOfferBalances().at(context.Height)) {
                    Amount mosaicAmountGive = expiredPair.second.InitialMosaicGive;
                    Amount mosaicAmountGet =  expiredPair.second.InitialMosaicGet;
                    std::string reduced = reducedFraction(mosaicAmountGive, mosaicAmountGet);
                    auto groupHash = calculateGroupHash(expiredPair.first.first, expiredPair.first.second, reduced);

                    auto& groupCache = context.Cache.sub<cache::SdaOfferGroupCache>();
                    auto groupIter = groupCache.find(groupHash);
                    auto& groupEntry = groupIter.get();

                    groupEntry.removeSdaOfferFromGroup(key);

                    if (groupEntry.empty())
                        groupCache.remove(groupHash);
                }

                // Immediately remove expired offers from the current cache Height
                auto pruneHeight = context.Height;
                auto& expiredOffers = entry.expiredSdaOfferBalances();
                if (expiredOffers.count(pruneHeight)) {
                    expiredOffers.at(pruneHeight).clear();
                    expiredOffers.erase(pruneHeight);
                }

                cache.removeExpiryHeight(key, context.Height);

                if (entry.empty())
                    cache.remove(key);
            }
        }))
    }
}}