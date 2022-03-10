/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/cache/ExchangeCache.h"

namespace catapult { namespace observers {

    DEFINE_OBSERVER(RemoveSdaExchangeOfferV1, model::RemoveSdaOfferNotification<1>, [](const model::RemoveSdaOfferNotification<1>& notification, const ObserverContext& context) {
        auto& cache = context.Cache.sub<cache::SdaExchangeCache>();
		auto iter = cache.find(notification.Owner);
		auto& entry = iter.get();
		SdaOfferExpiryUpdater sdaOfferExpiryUpdater(cache, entry);

        auto pSdaOffer = notification.SdaOffersPtr;
        for (uint8_t i = 0; i < notification.SdaOfferCount; ++i, ++pSdaOffer) {
            auto mosaicId = context.Resolvers.resolve(pSdaOffer->MosaicId);
            Amount amount = entry.swapOffers().at(mosaicId).ResidualMosaicGet;
            CreditAccount(entry.owner(), mosaicId, amount, context);
            entry.expireOffer(mosaicId, context.Height);
		}
    });
}}
