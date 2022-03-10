/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"

namespace catapult { namespace observers {

	namespace {
		Height GetOfferDeadline(const BlockDuration& duration, const Height& currentHeight) {
			auto maxDeadline = std::numeric_limits<Height::ValueType>::max();
			return Height((duration.unwrap() < maxDeadline - currentHeight.unwrap()) ? currentHeight.unwrap() + duration.unwrap() : maxDeadline);
		}

        template<typename TOfferMap>
		state::SdaOfferBase& ModifyOffer(TOfferMap& offers, const MosaicId& mosaicIdGive, const MosaicId& mosaicIdGet, const model::SdaOfferWithOwnerAndDuration* pSdaOffer) {
			auto& offer = offers.at(mosaicIdGive);
			offer -= *pSdaOffer;

            offer = offers.at(mosaicIdGet);
            offer += *pSdaOffer;
			
            return dynamic_cast<state::SdaOfferBase&>(offer);
		}
	}

    DEFINE_OBSERVER(PlaceSdaExchangeOfferV1, model::PlaceSdaOfferNotification<1>, [](const model::PlaceSdaOfferNotification<1>& notification, const ObserverContext& context) {
        auto& cache = context.Cache.sub<cache::SdaExchangeCache>();
        if (!cache.contains(notification.Signer))
            cache.insert(state::SdaExchangeEntry(notification.Signer, 1));
        
        auto iter = cache.find(notification.Signer);
        auto& entry = iter.get();
        SdaOfferExpiryUpdater sdaOfferExpiryUpdater(cache, entry);

        const auto* pSdaOffer = notification.SdaOffersPtr;
        for (uint8_t i = 0; i < notification.SdaOfferCount; ++i, ++pSdaOffer) {
            auto mosaicId = context.Resolvers.resolve(pSdaOffer->MosaicGive.MosaicId);
            auto deadline = GetOfferDeadline(pSdaOffer->Duration, context.Height);
            entry.addOffer(mosaicId, pSdaOffer, deadline);
        }

        if (notification.OwnerOfSdaOfferToExchangeWith.size() != 0) {
            auto iter = cache.find(notification.OwnerOfSdaOfferToExchangeWith);
            auto& entry = iter.get();
            SdaOfferExpiryUpdater sdaOfferExpiryUpdater(cache, entry);

            for (uint8_t i = 0; i < notification.SdaOfferCount; ++i, ++pSdaOffer) {
                auto mosaicIdGive = context.Resolvers.resolve(pSdaOffer->MosaicGive.MosaicId);
                auto mosaicIdGet = context.Resolvers.resolve(pSdaOffer->MosaicGet.MosaicId);
                auto& offer = ModifyOffer(entry.swapOffers(), mosaicIdGive, mosaicIdGet, pSdaOffer);
                if (Amount(0) == offer.CurrentMosaicGive)
                    entry.expireOffer(mosaicIdGive, context.Height);
            }
        }
    });
}}