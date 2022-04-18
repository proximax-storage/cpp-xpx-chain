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

            Amount mosaicGiveAmount = entry.sdaOfferBalances().find(state::MosaicsPair{mosaicIdGive, mosaicIdGet})->second.CurrentMosaicGive;
            Amount mosaicGetAmount = entry.sdaOfferBalances().find(state::MosaicsPair{mosaicIdGive, mosaicIdGet})->second.CurrentMosaicGet;
            CreditAccount(entry.owner(), mosaicIdGive, mosaicGiveAmount, context);

            std::string reduced = reducedFraction(mosaicGiveAmount, mosaicGetAmount);
            auto groupHash = calculateGroupHash(mosaicIdGive, mosaicIdGet, reduced);

            auto& groupCache = context.Cache.sub<cache::SdaOfferGroupCache>();
            auto groupIter = groupCache.find(groupHash);
            auto& groupEntry = groupIter.get();

            entry.expireOffer(state::MosaicsPair{mosaicIdGive,mosaicIdGet}, context.Height);
            groupEntry.removeSdaOfferFromGroup(notification.Owner);
        }
    });
}}
