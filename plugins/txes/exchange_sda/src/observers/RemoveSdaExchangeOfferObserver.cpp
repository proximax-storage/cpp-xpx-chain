/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/cache/SdaExchangeCache.h"
#include "src/cache/SdaOfferGroupCache.h"

namespace catapult { namespace observers {

    DEFINE_OBSERVER(RemoveSdaExchangeOfferV1, model::RemoveSdaOfferNotification<1>, [](const model::RemoveSdaOfferNotification<1>& notification, const ObserverContext& context) {
        auto& cache = context.Cache.sub<cache::SdaExchangeCache>();
        auto iter = cache.find(notification.Owner);
        auto& entry = iter.get();
        SdaOfferExpiryUpdater sdaOfferExpiryUpdater(cache, entry);

        auto pSdaOffer = notification.SdaOffersPtr;
        for (uint8_t i = 0; i < notification.SdaOfferCount; ++i, ++pSdaOffer) {
            auto mosaicIdGive = context.Resolvers.resolve(pSdaOffer->MosaicIdGive);
            auto mosaicIdGet = context.Resolvers.resolve(pSdaOffer->MosaicIdGet);

            Amount mosaicAmountGive = entry.sdaOfferBalances().find(state::MosaicsPair{mosaicIdGive, mosaicIdGet})->second.InitialMosaicGive;
            Amount mosaicAmountGet = entry.sdaOfferBalances().find(state::MosaicsPair{mosaicIdGive, mosaicIdGet})->second.InitialMosaicGet;

            std::string reduced = reducedFraction(mosaicAmountGive, mosaicAmountGet);
            auto groupHash = calculateGroupHash(mosaicIdGive, mosaicIdGet, reduced);

            auto& groupCache = context.Cache.sub<cache::SdaOfferGroupCache>();
            if (!groupCache.contains(groupHash))
                continue;
            auto groupIter = groupCache.find(groupHash);
            auto& groupEntry = groupIter.get();

            CreditAccount(entry.owner(), mosaicIdGive, mosaicAmountGive, context);
            entry.expireOffer(state::MosaicsPair{mosaicIdGive,mosaicIdGet}, context.Height);
            groupEntry.removeSdaOfferFromGroup(notification.Owner);
        }

        if (entry.empty())
            cache.remove(notification.Owner);
    });
}}
