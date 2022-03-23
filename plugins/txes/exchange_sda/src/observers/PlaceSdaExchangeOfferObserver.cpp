/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include <boost/lexical_cast.hpp>

namespace catapult { namespace observers {

	namespace {
		Height GetOfferDeadline(const BlockDuration& duration, const Height& currentHeight) {
			auto maxDeadline = std::numeric_limits<Height::ValueType>::max();
			return Height((duration.unwrap() < maxDeadline - currentHeight.unwrap()) ? currentHeight.unwrap() + duration.unwrap() : maxDeadline);
		}

        template<typename TOfferMap>
		state::SdaOfferBalance& ModifyOffer(TOfferMap& offers, const MosaicId& mosaicIdGive, const MosaicId& mosaicIdGet, const model::SdaOfferWithOwnerAndDuration* pSdaOffer) {
			auto& offer = offers.at(mosaicIdGive);
            *pSdaOffer.sender(offer);
			
            offer = offers.at(mosaicIdGet);
            offer.receiver(*pSdaOffer);
	
            return dynamic_cast<state::SdaOfferBalance&>(offer);
		}
	}

    DEFINE_OBSERVER(PlaceSdaExchangeOfferV1, model::PlaceSdaOfferNotification<1>, [](const model::PlaceSdaOfferNotification<1>& notification, const ObserverContext& context) {
        auto& cache = context.Cache.sub<cache::SdaExchangeCache>();
        if (!cache.contains(notification.Signer))
            cache.insert(state::SdaExchangeEntry(notification.Signer, 1));
        
        auto iter = cache.find(notification.Signer);
        auto& entry = iter.get();
        SdaOfferExpiryUpdater sdaOfferExpiryUpdater(cache, entry);

        /// Save offers in cache
        const auto* pSdaOffer = notification.SdaOffersPtr;
        for (uint8_t i = 0; i < notification.SdaOfferCount; ++i, ++pSdaOffer) {
            auto mosaicGiveId = context.Resolvers.resolve(pSdaOffer->MosaicGive.MosaicId);
            auto mosaicGetId = context.Resolvers.resolve(pSdaOffer->MosaicGet.MosaicId);

            auto deadline = GetOfferDeadline(pSdaOffer->Duration, context.Height);
            entry.addOffer(mosaicGiveId, pSdaOffer, deadline);
            
            std::string reduced = reducedFraction(pSdaOffer->MosaicGive.Amount, pSdaOffer->MosaicGet.Amount);
            auto groupHash = calculateGroupHash(mosaicGiveId, mosaicGetId, reduced);

            auto& groupCache = context.Cache.sub<cache::SdaOfferGroupCache>();
            if (!groupCache.contains(groupHash))
                groupCache.insert(state::SdaOfferGroupEntry(groupHash));

            auto groupIter = groupCache.find(groupHash);
            auto& groupEntry = groupIter.get();

            groupEntry.addSdaOfferToGroup(groupHash, pSdaOffer, deadline);
        }

        /// Exchange offers when a match is found in cache
        for (uint8_t i = 0; i < notification.SdaOfferCount; ++i, ++pSdaOffer) {
            auto mosaicIdGive = context.Resolvers.resolve(pSdaOffer->MosaicGive.MosaicId);
            auto mosaicIdGet = context.Resolvers.resolve(pSdaOffer->MosaicGet.MosaicId);

            std::string reduced = reducedFraction(pSdaOffer->MosaicGive.Amount, pSdaOffer->MosaicGet.Amount);
            auto groupHash = calculateGroupHash(mosaicIdGive, mosaicIdGet, reduced);

            auto& groupCache = context.Cache.sub<cache::SdaOfferGroupCache>();
            auto groupIter = groupCache.find(groupHash);
            auto& groupEntry = groupIter.get();

            auto& offer = ModifyOffer(entry.sdaOfferBalances(), mosaicIdGive, mosaicIdGet, pSdaOffer);
            if (Amount(0) == offer.CurrentMosaicGive) {
                entry.expireOffer(mosaicIdGive, context.Height);
            }
        }
    });
}}